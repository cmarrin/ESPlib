/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebFileSystem.h"

#include "Application.h"
#include "HTTPParser.h"

#include <filesystem>
#include "lua.hpp"

using namespace mil;

// There are a couple of bare "const"

#define USE_GZIP_HTML
#ifdef USE_GZIP_HTML
const 
#include "landing.gz.h"
const 
#include "wifi.gz.h"
const 
#include "filemgr.gz.h"
#define HTML_IS_GZIP true
#define LANDING_NAME landing_html_gz
#define LANDING_LEN_NAME landing_html_gz_len
#define WIFI_NAME wifi_html_gz
#define WIFI_LEN_NAME wifi_html_gz_len
#define FILEMGR_NAME filemgr_html_gz
#define FILEMGR_LEN_NAME filemgr_html_gz_len
#else
const 
#include "landing.h"
const 
#include "wifi.h"
const 
#include "filemgr.h"
#define HTML_IS_GZIP false
#define LANDING_NAME landing_html
#define LANDING_LEN_NAME landing_html_len
#define WIFI_NAME wifi_html
#define WIFI_LEN_NAME wifi_html_len
#define FILEMGR_NAME filemgr_html
#define FILEMGR_LEN_NAME filemgr_html_len
#endif

#ifndef ARDUINO
fs::FS LittleFS;
#endif

// If return is true path has path to use and the file or dir exists
bool
WebFileSystem::prepareFile(WiFiPortal* p, std::string& path)
{
    path = HTTPParser::urlDecode(p->getHTTPArg("path"));
    
    if (path.empty()) {
        p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
        return false;
    }
    if (!exists(path.c_str())) {
        p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Not found\"}");
        return false;
    }
    return true;
}

void
WebFileSystem::sendLandingPage(WiFiPortal* portal)
{
    portal->sendHTTPResponse(200, "text/html", reinterpret_cast<const char*>(LANDING_NAME), LANDING_LEN_NAME, HTML_IS_GZIP);
}

void
WebFileSystem::sendWiFiPage(WiFiPortal* portal)
{
    portal->sendHTTPResponse(200, "text/html", reinterpret_cast<const char*>(WIFI_NAME), WIFI_LEN_NAME, HTML_IS_GZIP);
}

bool
WebFileSystem::begin(Application* app, bool format)
{
    app->addHTTPHandler("/get-landing-setup", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { handleLandingSetup(p); });
    
    app->addHTTPHandler("/filemgr", [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(200, "text/html", reinterpret_cast<const char*>(FILEMGR_NAME), FILEMGR_LEN_NAME, HTML_IS_GZIP);
        return true;
    });

    app->addHTTPHandler("/get-folder-contents", [this](WiFiPortal* p)
    {
        std::string s = "0,";
        s += std::to_string(totalBytes()) + "," + std::to_string(usedBytes());
        std::string dir = listDir(HTTPParser::urlDecode(p->getHTTPArg("path")).c_str(), 0);
        if (!dir.empty()) {
            s += ":";
            s += dir;
        }
        p->sendHTTPResponse(200, "text/html", s.c_str());
        return true;
    });

    app->addHTTPHandler("/newfolder", [this](WiFiPortal* p)
    {
        std::string path = HTTPParser::urlDecode(p->getHTTPArg("path"));
        
        if (path.empty()) {
            p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
        } else if (exists(path.c_str())) {
            p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Folder already exists\"}");
        } else {
            if (mkdir(path.c_str())) {
                p->sendHTTPResponse(200, "application/json", "{\"status\":\"success\",\"message\":\"Folder created successfully\"}");
            } else {
                p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Could not create folder\"}");
            }
        }
        return true;
    });

    app->addHTTPHandler("/delete", [this](WiFiPortal* p)
    {
        std::string path;
        if (prepareFile(p, path)) {
            fs::File file = open(path.c_str(), "r");
            if (file.isDirectory()) {
                file.close();
                if (LittleFS.rmdir(path.c_str())) {
                    p->sendHTTPResponse(200, "application/json", "{\"status\":\"success\",\"message\":\"Directory deleted successfully\"}");
                } else {
                    p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Could not delete directory\"}");
                }
            } else {
                file.close();
                if (LittleFS.remove(path.c_str())) {
                    p->sendHTTPResponse(200, "application/json", "{\"status\":\"success\",\"message\":\"File deleted successfully\"}");
                } else {
                    p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Could not delete file\"}");
                }
            }
        }
        return true;
    });

    // Open file (downloads with "attachment" disposition) - this downloads the file to the client
    app->addHTTPHandler("/download", [this](WiFiPortal* p)
    {
        std::string path;
        if (prepareFile(p, path)) {
            fs::File file = open(path.c_str(), "r");
            p->streamHTTPResponse(file, "application/octet-stream", true);
        }
        return true;
    });

    // Open file (downloads with "inline" disposition) - this displays the content in a web page
    app->addHTTPHandler("/file", [this](WiFiPortal* p)
    {
        std::string path;
        if (prepareFile(p, path)) {
            fs::File file = open(path.c_str(), "r");
            std::string mime = HTTPParser::suffixToMimeType(path);
            printf("***** File: path='%s', mime-type='%s'\n", path.c_str(), mime.c_str());
                
            if (mime.empty()) {
                p->sendHTTPResponse(404, "text/html", "<center><h1>File cannot be displayed</h1><h2>Use Download</h2></center>");
            } else {
                p->streamHTTPResponse(file, mime.c_str(), false);
            }
        }
        return true;
    });

    // Run the file if it is .lua or .luac
    app->addHTTPHandler("/run", [this](WiFiPortal* p)
    {
        std::string path;
        if (prepareFile(p, path)) {
            std::string suffix = std::filesystem::path(path).extension();
            if (suffix != ".lua" && suffix != ".luac") {
                p->sendHTTPResponse(404, "text/html", "<center><h1>File cannot be run</h1><h2>Can only run .lua and .luac files</h2></center>");
                return true;
            }
            
            if (_lua.execute(realPath(path).c_str()) != LUA_OK) {
                printf("%s\n", _lua.toString(-1));
                std::string err = "Lua file '" + path + "' failed to run: " + _lua.toString(-1) + "\n";
                p->sendHTTPResponse(404, "text/plain", err.c_str());
            } else {
                printf("***** Running Lua file '%s'\n", path.c_str());
                p->sendHTTPResponse(200, "text/html", "<center><h1>Lua Runtime</h1><h2>No output</h2></center>");
            }
        }
        return true;
    });

    app->addHTTPHandler("/upload", WiFiPortal::HTTPMethod::Post, [this](WiFiPortal* p) { handleUpload(p); });

    bool retval = LittleFS.begin(format);
    if (!retval) {
        printf("***** error mounting littlefs\n");
    }
    
    return retval;
}

size_t
WebFileSystem::totalBytes()
{
    return LittleFS.totalBytes();
}

size_t
WebFileSystem::usedBytes()
{
    return LittleFS.usedBytes();
}

bool
WebFileSystem::exists(const char* path)
{
    return LittleFS.exists(path);
}

bool
WebFileSystem::remove(const char* path)
{
    return LittleFS.remove(path);
}

bool
WebFileSystem::rename(const char* fromPath, const char* toPath)
{
    return LittleFS.rename(fromPath, toPath);
}

bool
WebFileSystem::mkdir(const char* path)
{
    return LittleFS.mkdir(path);
}

bool
WebFileSystem::rmdir(const char* path)
{
    return LittleFS.rmdir(path);
}

std::string
WebFileSystem::listDir(const char* dirname, uint8_t levels)
{
    std::string s;
    
    fs::File root = open(dirname);
    if (!root){
        printf("Failed to open directory\n");
        return "";
    }
    
    if (!root.isDirectory()){
        printf("Not a directory\n");
        root.close();
        return "";
    }

    bool first_files = true;
    fs::File file = root.openNextFile();
    while(file){
        if (first_files)
            first_files = false;
        else 
            s += ":";

        if (file.isDirectory()) {
            s += "1,";
            s += file.name();
        } else {
            s += "0,";
            s += file.name();
            s += ",";
            s += std::to_string(file.size());
        }
        
        file = root.openNextFile();
    }

    file.close();
    
    return s;
}

void
WebFileSystem::handleUpload(WiFiPortal* p)
{
    bool finished = false;
    
    switch(p->httpUploadStatus()) {
        case WiFiPortal::HTTPUploadStatus::Start:
            _uploadFilename = HTTPParser::urlDecode(p->getHTTPArg("path")) + "/" + p->httpUploadFilename();
            _uploadAborted = false;
        
            // Open file for writing
            _uploadFile = open(_uploadFilename.c_str(), "w");
            if (!_uploadFile) {
                printf("Failed to open file for writing\n");
                _uploadAborted = true;
            }
            break;
        case WiFiPortal::HTTPUploadStatus::Write:
            if (!_uploadAborted) {
                // Write the received chunk to the file
                if (_uploadFile) {
                    size_t currentSize = p->httpUploadCurrentSize();
                    size_t bytesWritten = _uploadFile.write(p->httpUploadBuffer(), currentSize);
                    
                    if (!_uploadFile || bytesWritten != currentSize) {
                        printf("Error writing chunk to file. Deleting '%s'\n", _uploadFilename.c_str());
                        
                        // Delete the file
                        _uploadFile.close();
                        remove(_uploadFilename.c_str());
                        _uploadAborted = true;
                        return;
                    }
                }
            }
            break;
        case WiFiPortal::HTTPUploadStatus::End:
            if (_uploadFile) {
                _uploadFile.close();
            } else {
                printf("handleUpload: END received but file not open.\n");
            }
            finished = true;
            break;
        case WiFiPortal::HTTPUploadStatus::Aborted:
            printf("handleUpload: Upload Aborted\n");
            if (_uploadFile) {
               // Delete the file
                std::string filename = _uploadFile.name();
                _uploadFile.close();
                remove(filename.c_str());
            }
            _uploadAborted = true;
            finished = true;
            break;
        case WiFiPortal::HTTPUploadStatus::None:
            printf("Invalid upload status\n");
            _uploadAborted = true;
            finished = true;
            break;
    }
    
    if (finished) {
        if (_uploadAborted) {
            p->sendHTTPResponse(500, "text/plain", "Upload Aborted");
            printf("Reply sent: Upload Aborted\n");
        } else if (p->httpUploadTotalSize() > 0) { // Check if any bytes were received
            p->sendHTTPResponse(200, "text/plain", "Upload Successful!");
            printf("Reply sent: Successful uploaded to '%s'\n", _uploadFilename.c_str());
        } else {
            // This might happen if the file was empty or write failed early
            p->sendHTTPResponse(500, "text/plain", "Upload Failed or Empty File");
            printf("Reply sent: Upload Failed/Empty\n");
        }
    }
}

std::string
WebFileSystem::makeJSON(std::vector<std::pair<std::string, std::string>> json)
{
    std::string s = "{";
    bool first = true;
    for (const auto& it : json) {
        if (first) {
            first = false;
        } else {
            s += ",";
        }
        s += jsonParam(it.first, it.second);
    }
    s += "}";
    return s;
}

void
WebFileSystem::handleLandingSetup(WiFiPortal* portal)
{
    std::string response = makeJSON(
        {
            { "title", portal->getTitle() },
            { "ssid", portal->getSSID() },
            { "ip", portal->getIP() },
            { "hostname", portal->getHostname() },
            { "gw", portal->getGateway() },
            { "msk", portal->getMask() },
            { "dns", portal->getDNS() },
            { "cpuModel", portal->getCPUModel() },
            { "cpuFreq", std::to_string(portal->getCPUFrequency()) },
            { "cpuTemp", std::to_string(portal->getCPUTemperature()) },
            { "cpuUptime", std::to_string(portal->getCPUUptime()) },
            { "flashTotal", std::to_string(totalBytes()) },
            { "flashUsed", std::to_string(usedBytes()) },
            { "customMenuHTML", portal->getCustomMenuHTML() }
        }
    );
    portal->sendHTTPResponse(200, "application/json", response.c_str(), response.length(), false);
}

fs::File
WebFileSystem::open(const char* path, const char* mode, bool create)
{
    return LittleFS.open(path, mode);
}

std::string
WebFileSystem::realPath(const std::string& path)
{
#if defined ARDUINO
    return "/littlefs" + path;
#else
    return LittleFS.makePath(path.c_str());
#endif
}
