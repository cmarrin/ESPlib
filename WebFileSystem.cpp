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

std::string WebFileSystem::_cwd = "/";
static const char* TAG = "WebFileSystem";

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

static std::string makeRedirectPage(const char* text)
{
    std::string s = "<!DOCTYPE html><html><head><title>Redirecting...</title>";
    s += "<meta http-equiv=\"refresh\" content=\"3; url=/\">";
    s += "</head><body>";
    s += text;
    s += "</body></html>";
    return s;
}

bool
WebFileSystem::begin(Application* app, bool format)
{
    bool retval = LittleFS.begin(format);
    if (!retval) {
        System::logE(TAG, "***** error mounting littlefs");
    }

    app->addHTTPHandler("/", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p)
    {
        if (p->isProvisioning()) {
            sendWiFiPage(p);
        } else {
            sendLandingPage(p);
        }
    });
    app->addHTTPHandler("/get-landing-setup", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { handleLandingSetup(p); });
    app->addHTTPHandler("/wifi", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { sendWiFiPage(p); });
    app->addHTTPHandler("/get-wifi-setup", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { handleWiFiSetup(p); });
    app->addHTTPHandler("/connect", WiFiPortal::HTTPMethod::Post, [this](WiFiPortal* p) { handleConnect(p); });
    app->addHTTPHandler("/update", WiFiPortal::HTTPMethod::Post, [this](WiFiPortal* p) { p->otaUpdate(); });

    app->addHTTPHandler("/restart", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(200, "text/html", makeRedirectPage("<h1>Restarting...</h1>").c_str());
        System::delay(2000);
        System::restart();
    });
    
    app->addHTTPHandler("/reset", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(200, "text/html", makeRedirectPage("<h1>Credentials Cleared</h1><p>The device will restart and enter provisioning mode.</p>").c_str());
        System::delay(2000);
        p->eraseNVSParam("wifi_ssid");
        p->eraseNVSParam("wifi_pass");
        System::restart();
    });

    app->addHTTPHandler("/favicon.ico", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(204, "text/plain", "No Content");
    });
    
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
                if (rmdir(path.c_str())) {
                    p->sendHTTPResponse(200, "application/json", "{\"status\":\"success\",\"message\":\"Directory deleted successfully\"}");
                } else {
                    p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Could not delete directory\"}");
                }
            } else {
                file.close();
                if (remove(path.c_str())) {
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
            System::logI(TAG, "File: path='%s', mime-type='%s'", path.c_str(), mime.c_str());
                
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
            std::string suffix = extension(path);
printf("********** run: path='%s', suffix='%s'\n", path.c_str(), suffix.c_str());
            if (suffix != "lua" && suffix != "luac") {
                p->sendHTTPResponse(404, "text/html", "<center><h1>File cannot be run</h1><h2>Can only run .lua and .luac files</h2></center>");
                return true;
            }
            
            LuaManager::execute(path.c_str());
            std::string response = "Running Lua file '" + path + "'";
            System::logI(TAG, response.c_str());
            response = "<center><h1>" + response + "</h1></center>";
            p->sendHTTPResponse(200, "text/html", response.c_str());
        }
        return true;
    });

    app->addHTTPHandler("/upload", WiFiPortal::HTTPMethod::Post, [this](WiFiPortal* p) { handleUpload(p); });
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
    return LittleFS.exists(realPath(path).c_str());
}

bool
WebFileSystem::remove(const char* path)
{
    return LittleFS.remove(realPath(path).c_str());
}

bool
WebFileSystem::rename(const char* fromPath, const char* toPath)
{
    return LittleFS.rename(realPath(fromPath).c_str(), realPath(toPath).c_str());
}

bool
WebFileSystem::mkdir(const char* path)
{
    return LittleFS.mkdir(realPath(path).c_str());
}

bool
WebFileSystem::rmdir(const char* path)
{
    return LittleFS.rmdir(realPath(path).c_str());
}

std::string
WebFileSystem::listDir(const char* dirname, uint8_t levels)
{
    std::string s;
    
    // First see if it's a directory
    fs::File root = open(dirname);
    if (!root){
        System::logE(TAG, "Failed to open directory\n");
        return "";
    }
    
    if (!root.isDirectory()){
        printf("Not a directory\n");
        root.close();
        return "";
    }
    
    bool first_files = true;
    
    while (true) {
        fs::File f = root.openNextFile();
        std::string path = f.name();
        if (path.empty()) {
            break;
        }

        if (first_files)
            first_files = false;
        else 
            s += ":";

        if (f.isDirectory()) {
            s += "1,";
            s += path;
        } else {
            s += "0,";
            s += path;
            s += ",";
            s += std::to_string(f.size());
        }
    }
    
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
    std::string customMenu;
    if (exists("/sys/shell.html")) {
        customMenu += "<form action='/fs/sys/shell.html' method='get'><button class='btn'>Shell</button></form><br/>";
    }

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
            { "customMenuHTML", customMenu },
            { "customInfoHTML", portal->getCustomInfoHTML() }
        }
    );
    portal->sendHTTPResponse(200, "application/json", response.c_str(), response.length(), false);
}

void
WebFileSystem::handleWiFiSetup(WiFiPortal* portal)
{
    std::string ssid, hostname;
    portal->getNVSParam("wifi_ssid", ssid);
    portal->getNVSParam("hostname", hostname);

    std::string response = "{" + jsonParam("ssid", ssid) + "," + jsonParam("hostname", hostname) + ",";
    response += quote("knownNetworks") + ":[";

    const std::vector<WiFiPortal::KnownNetwork>* knownNetworks = portal->getKnownNetworks();
    
    if (knownNetworks) {
        bool first = true;
        
        for (const auto& it : *knownNetworks) {
            if (!first) {
                response += ",";
            } else {
                first = false;
            }
            
            response += "{" + jsonParam("ssid", it.ssid) + ",";
            response += jsonParam("rssi", std::to_string(it.rssi)) + ",";
            response += jsonParam("open", std::string(it.open ? "true" : "false")) + "}";
        }
    }
    
    response += "],";
    response += quote("customTextForms") + ":";
    response += portal->getCustomTextForms();
    response += "}";

    portal->sendHTTPResponse(200, "application/json", response.c_str(), response.length(), false);
}

void
WebFileSystem::handleConnect(WiFiPortal* portal)
{
    std::string sizeString = portal->getHTTPHeader("Content-Length");
    size_t size = strtol(sizeString.c_str(), nullptr, 10);
    if (size == 0) {
        // Some default value
        size = 100;
    }
    char* buf = new char[size + 1];
    buf[0] = '\0';
    
    int actualSize = portal->receiveHTTPResponse(buf, size);
    buf[actualSize] = '\0';

    portal->parseQuery(buf);
    delete [ ] buf;

    // Decode the strings
    std::string ssid = HTTPParser::urlDecode(portal->getHTTPArg("ssid"));
    std::string pass = HTTPParser::urlDecode(portal->getHTTPArg("password"));
    std::string hostname = HTTPParser::urlDecode(portal->getHTTPArg("hostname"));
    
    if (ssid.empty()) {
        portal->getNVSParam("wifi_ssid", ssid);
        portal->getNVSParam("wifi_pass", pass);
    }
    
    if (hostname.empty()) {
        portal->getNVSParam("hostname", hostname);
    }

    std::string response = "<h1>Connecting to '";
    response += ssid;
    response += "'...</h1><p>If successful, the device will connect to the network. If failed, it will remain in provisioning mode.</p>";
    portal->sendHTTPResponse(200, "text/html", makeRedirectPage(response.c_str()).c_str());

    portal->setNVSParam("wifi_ssid", ssid);
    portal->setNVSParam("wifi_pass", pass);
    portal->setNVSParam("hostname", hostname);

    // Set the custom values
    for (uint32_t i = 0; ;++i) {
        std::string id;
        if (!portal->getFormEntryId(i, id)) {
            break;
        }
        std::string value = HTTPParser::urlDecode(portal->getHTTPArg(id.c_str()));
        portal->setNVSParam(id.c_str(), value);
    }

    System::delay(1000);
    System::restart();
}

fs::File
WebFileSystem::open(const char* path, const char* mode, bool create)
{
    return LittleFS.open(realPath(path).c_str(), mode);
}

std::string
WebFileSystem::realPath(const char* path)
{
    std::string p;
    if (path == nullptr || path[0] == '\0') {
        // Empty path. just use _cwd
        p = _cwd;
    } else {
        if (path[0] != '/') {
            p = _cwd;
        }
        p += std::string("/") + path;
    }

#if defined ARDUINO
    p = "/littlefs" + p;
#else
    p = LittleFS.rootDir() + p;
#endif
    return lexicallyNormal(p);
}

std::string
WebFileSystem::lexicallyNormal(const std::string& path)
{
    // We only deal with absolute paths
    if (path.empty() || path[0] != '/') {
        return path;
    }
    
    // First split the string into a vector of strings, splitting at
    // slashes. For instance:
    //
    //      '/abc/def/../ghi/.//foo.bar'
    //
    // Would produce:
    //
    //      "abc", "def", "..", "ghi", ".", "foo.bar"
    //
    std::vector<std::string> parts;
    size_t cur = 1; // Skip the leading slash
    
    while (true) {
        const char* slash = std::strchr(path.c_str() + cur, '/');
        if (!slash) {
            break;
        }

        // Ignore "." and ""
        size_t pos = slash - path.c_str();
        if (pos == cur || ((pos == (cur + 1)) && path[cur] == '.')) {
            cur = pos + 1;
            continue;
        }
        
        std::string part = path.substr(cur, pos - cur);

        // Back up one directory on "..". There's an edge case. What does
        // '/../foo' mean? Just ignore the ".." in that case
        if (part == "..") {
            if (parts.size() > 0) {
                parts.pop_back();
            }
            cur = pos + 1;
            continue;
        }
        parts.push_back(part);
        cur = pos + 1;
    }
    
    // Now assemble the path
    std::string dest;
    for (const auto& s : parts) {
        dest += "/" + s;
    }
    
    // Add in the trailing part
    dest += "/" + path.substr(cur);
    
    return dest;
}
