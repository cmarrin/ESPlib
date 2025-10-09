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
FS LittleFS;
#endif

std::string
WebFileSystem::listDir(const char* dirname, uint8_t levels)
{
    std::string s;
    
    printf("Listing directory: %s\n", dirname);

    File root = open(dirname);
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
    File file = root.openNextFile();
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

// Returns null if file is not a source file (cannot be displayed in web browser)
static std::string suffixToMimeType(const std::string& filename)
{
    std::string suffix = std::filesystem::path(filename).extension();
    for_each(suffix.begin(), suffix.end(), [](char& c) {
        c = std::tolower(c);
    });

    printf("***** suffixToMimeType: suffix='%s'\n", suffix.c_str());

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

bool
WebFileSystem::begin(Application* app, bool format)
{
    app->setCustomMenuHTML("<form action='/fs' method='get'><button>File Manager</button></form><br/>\n");

    app->addHTTPHandler("/fs", [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(200, "text/html", reinterpret_cast<const char*>(FILEMGR_NAME), FILEMGR_LEN_NAME, FILEMGR_IS_GZIP);
        return true;
    });

    app->addHTTPHandler("/get-folder-contents", [this](WiFiPortal* p)
    {
        std::string s = "0,";
        s += std::to_string(LittleFS.totalBytes()) + "," + std::to_string(LittleFS.usedBytes());
        std::string dir = listDir(p->getHTTPArg("path").c_str(), 0);
        if (!dir.empty()) {
            s += ":";
            s += dir;
        }
        p->sendHTTPResponse(200, "text/html", s.c_str());
        return true;
    });

    app->addHTTPHandler("/newfolder", [this](WiFiPortal* p)
    {
        std::string path = p->getHTTPArg("path");
        
        printf("***** creating folder at '%s'\n", path.c_str());
        
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
        std::string path = p->getHTTPArg("path");
        
        if (path.empty()) {
            p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
        } else if (!LittleFS.exists(path.c_str())) {
            p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Not found\"}");
        } else {
            File file = open(path.c_str(), "r");
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
        std::string path = p->getHTTPArg("path");
        
        if (path.empty()) {
            p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
        } else if (!LittleFS.exists(path.c_str())) {
            p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"File not found\"}");
        } else {
            File file = open(path.c_str(), "r");
            if (!file) {
                p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"File not found\"}");
            } else {
                printf("***** Download: path='%s', name='%s'\n", file.path(), file.name());
                p->streamHTTPResponse(file, "application/octet-stream", true);
            }
        }
        return true;
    });

    // Open file (downloads with "inline" disposition)
    app->addHTTPHandler("/file", [this](WiFiPortal* p)
    {
        std::string path = p->getHTTPArg("path");
        
        if (path.empty()) {
            p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Path not provided\"}");
        } else if (!LittleFS.exists(path.c_str())) {
            p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"File not found\"}");
        } else {
            File file = open(path.c_str(), "r");
            if (!file) {
                p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"File not found\"}");
            } else {
                std::string mime = suffixToMimeType(path);
                printf("***** File: path='%s', mime-type='%s'\n", path.c_str(), mime.c_str());
                
                if (mime.empty()) {
                    p->sendHTTPResponse(404, "text/html", "<center><h1>File cannot be displayed</h1><h2>Use Download</h2></center>");
                } else {
                    p->streamHTTPResponse(file, mime.c_str(), false);
                }
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

void
WebFileSystem::handleUpload(WiFiPortal* p)
{
    if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Start) {
        _uploadFilename = p->httpUploadFilename();
        _uploadAborted = false;
        if (_uploadFilename[0] != '/') {
            _uploadFilename = "/" + _uploadFilename; // Prepend slash for SPIFFS path
        }
        printf("handleUpload: START, filename='%s'\n", _uploadFilename.c_str());
    
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
            
            if (bytesWritten != currentSize) {
                printf("Error writing chunk to file. Deleting '%s'\n", _uploadFilename.c_str());
                
                // Delete the file
                _uploadFile.close();
                remove(_uploadFilename.c_str());
                _uploadAborted = true;
                return;
            }
            printf("handleUpload: WRITE, Bytes: %d\n", int(currentSize));
        }
    } else if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::End) {
        if (_uploadFile) {
            printf("handleUpload: END, Size: %d\n", int(p->httpUploadTotalSize()));
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

File
WebFileSystem::open(const char* path, const char* mode, bool create)
{
    return LittleFS.open(path, mode);
}
