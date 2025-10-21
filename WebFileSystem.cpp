/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebFileSystem.h"

#include "Application.h"
#include <filesystem>

using namespace mil;

#define USE_GZIP_FILEMGR
#ifdef USE_GZIP_FILEMGR
#include "filemgr.gz.h"
#define FILEMGR_NAME filemgr_html_gz
#define FILEMGR_LEN_NAME filemgr_html_gz_len
#define FILEMGR_IS_GZIP true
#else
#include "filemgr.h"
#define FILEMGR_NAME filemgr_html
#define FILEMGR_LEN_NAME filemgr_html_len
#define FILEMGR_IS_GZIP false
#endif

#ifndef ARDUINO
fs::FS LittleFS;
#endif

static std::string urlDecode(const std::string& s)
{
    std::vector<std::string> parts = WebFileSystem::split(s, '%');
    std::string result = parts[0];
    parts.erase(parts.begin());
    
    for (const auto& it : parts) {
        // First 2 chars are the hex code
        assert(it.size() >= 2);
        
        char buf[3] = "XX";
        buf[0] = it[0];
        buf[1] = it[1];
        uint8_t hex = strtol(buf, nullptr, 16);
        result += char(hex);
        result += it.substr(2);
    }
    
    return result;
}

std::vector<std::string>
WebFileSystem::split(const std::string& str, char sep)
{
    std::vector<std::string> strings;
    
    int startIndex = 0;
    int endIndex = 0;
    
    for (int i = 0; i <= str.size(); i++) {
        if (str[i] == sep || i == str.size()) {
            endIndex = i;
            std::string temp;
            temp.append(str, startIndex, endIndex - startIndex);
            strings.push_back(temp);
            startIndex = endIndex + 1;
        }
    }
    return strings;
}

std::string
WebFileSystem::trimWhitespace(const std::string& s)
{
    std::string returnString(s);
    returnString.erase(0, returnString.find_first_not_of(" \t"));
    returnString.erase(returnString.find_last_not_of(" \t") + 1);
    return returnString;
}

std::string
WebFileSystem::removeQuotes(const std::string& s)
{            
    if (s[0] == '"') {
        std::string returnString = s.substr(1);
        returnString.pop_back();
        return returnString;
    }
    return s;
}

// Returns null if file is not a source file (cannot be displayed in web browser)
std::string
WebFileSystem::suffixToMimeType(const std::string& filename)
{
    std::string suffix = std::filesystem::path(filename).extension();
    for_each(suffix.begin(), suffix.end(), [](char& c) {
        c = std::tolower(c);
    });

    if (suffix == ".htm" || suffix == ".html") {
        return "text/html";
    } else if (suffix == ".css") {
        return "text/css";
    } else if (suffix == ".js") {
        return "text/javascript";
    } else if (suffix == ".png") {
        return "image/png";
    } else if (suffix == ".gif") {
        return "image/gif";
    } else if (suffix == ".jpg" || suffix == ".jpeg") {
        return "image/jpeg";
    } else if (suffix == ".pdf") {
        return "application/pdf";
    } else if (suffix == ".txt" || suffix == ".c" || suffix == ".h" || suffix == ".cpp" || suffix == ".hpp") {
        return "text/plain";
    } else {
        return "";
    }
}

// If return is true path has path to use and the file or dir exists
bool
WebFileSystem::prepareFile(WiFiPortal* p, std::string& path)
{
    path = urlDecode(p->getHTTPArg("path"));
    
    if (path.empty()) {
        p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
    } else if (!LittleFS.exists(path.c_str())) {
        p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Not found\"}");
    } else {
        if (exists(path.c_str())) {
            return true;
        }
        p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"File not found\"}");
    }
    return false;
}

bool
WebFileSystem::begin(Application* app, bool format)
{
    app->addHTTPHandler("/filemgr", [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(200, "text/html", reinterpret_cast<const char*>(FILEMGR_NAME), FILEMGR_LEN_NAME, FILEMGR_IS_GZIP);
        return true;
    });

    app->addHTTPHandler("/get-folder-contents", [this](WiFiPortal* p)
    {
        std::string s = "0,";
        s += std::to_string(LittleFS.totalBytes()) + "," + std::to_string(LittleFS.usedBytes());
        std::string dir = listDir(urlDecode(p->getHTTPArg("path")).c_str(), 0);
        if (!dir.empty()) {
            s += ":";
            s += dir;
        }
        p->sendHTTPResponse(200, "text/html", s.c_str());
        return true;
    });

    app->addHTTPHandler("/newfolder", [this](WiFiPortal* p)
    {
        std::string path = urlDecode(p->getHTTPArg("path"));
        
        if (path.empty()) {
            p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
        } else if (LittleFS.exists(path.c_str())) {
            p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Folder already exists\"}");
        } else {
            if (LittleFS.mkdir(path.c_str())) {
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

    // Open file (downloads with "attachment" disposition)
    app->addHTTPHandler("/download", [this](WiFiPortal* p)
    {
        std::string path;
        if (prepareFile(p, path)) {
            fs::File file = open(path.c_str(), "r");
            printf("***** Download: path='%s', name='%s'\n", file.path(), file.name());
            p->streamHTTPResponse(file, "application/octet-stream", true);
        }
        return true;
    });

    // Open file (downloads with "inline" disposition)
    app->addHTTPHandler("/file", [this](WiFiPortal* p)
    {
        std::string path;
        if (prepareFile(p, path)) {
            fs::File file = open(path.c_str(), "r");
            std::string mime = suffixToMimeType(path);
            printf("***** File: path='%s', mime-type='%s'\n", path.c_str(), mime.c_str());
                
            if (mime.empty()) {
                p->sendHTTPResponse(404, "text/html", "<center><h1>File cannot be displayed</h1><h2>Use Download</h2></center>");
            } else {
                p->streamHTTPResponse(file, mime.c_str(), false);
            }
        }
        return true;
    });
            }
        }
        return true;
    });

    app->addHTTPHandler("/upload", [this](WiFiPortal* p) { handleUploadFinished(p); }, [this](WiFiPortal* p) { handleUpload(p); });


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
    if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Start) {
        _uploadFilename = urlDecode(p->getHTTPArg("path")) + "/" + p->httpUploadFilename();
        _uploadAborted = false;
    
        // Open file for writing
        _uploadFile = open(_uploadFilename.c_str(), "w");
        if (!_uploadFile) {
            printf("Failed to open file for writing\n");
            return;
        }
    } else if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Write) {
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
    } else if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::End) {
        if (_uploadFile) {
            _uploadFile.close();
        } else {
            printf("handleUpload: END received but file not open.\n");
        }
    } else if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Aborted) {
        printf("handleUpload: Upload Aborted\n");
        if (_uploadFile) {
           // Delete the file
            std::string filename = _uploadFile.name();
            _uploadFile.close();
            remove(filename.c_str());
        }
    }
}

void
WebFileSystem::handleUploadFinished(WiFiPortal* p)
{
    if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Aborted || _uploadAborted) {
        p->sendHTTPResponse(500, "text/plain", "Upload Aborted");
        printf("Reply sent: Upload Aborted\n");
    } else if (p->httpUploadTotalSize() > 0) { // Check if any bytes were received
        p->sendHTTPResponse(200, "text/plain", "Upload Successful!");
        printf("Reply sent: Upload Successful\n");
    } else {
        // This might happen if the file was empty or write failed early
        p->sendHTTPResponse(500, "text/plain", "Upload Failed or Empty File");
        printf("Reply sent: Upload Failed/Empty\n");
    }
}

fs::File
WebFileSystem::open(const char* path, const char* mode, bool create)
{
    return LittleFS.open(path, mode);
}
