/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebFileSystem.h"

#include "Application.h"

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
            // DEBUGF("  DIR : ");
            // DEBUGL(file.name());
            s += "1,";
            s += file.name();
            // if(levels){
            //     listDir(file.path(), levels -1);
            // }
        } else {
            // DEBUG("  FILE: ");
            // DEBUG(file.name());
            // DEBUG("  SIZE: ");
            // DEBUGL(file.size());
            s += "0,";
            s += file.name();
        }
        
        file = root.openNextFile();
    }

    file.close();
    // DEBUGL2("Folder string ", s);
    
    return s;
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
        std::string s = listDir(p->getHTTPArg("path").c_str(), 0);
        p->sendHTTPResponse(200, "text/html", s.c_str());
        return true;
    });

    app->addHTTPHandler("/upload", [this](WiFiPortal* p) { handleUploadFinished(p); }, [this](WiFiPortal* p) { handleUpload(p); });

    return LittleFS.begin(format);
}

void
WebFileSystem::handleUpload(WiFiPortal* p)
{
    if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Start) {
        std::string filename = p->httpUploadFilename();
        if (filename[0] != '/') {
            filename = "/" + filename; // Prepend slash for SPIFFS path
        }
        printf("handleUpload: START, filename='%s'\n", filename.c_str());
    
        // Open file for writing
        _uploadFile = open(filename.c_str(), "w");
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
                printf("Error writing chunk to file\n");
                
                // Delete the file
                std::string filename = _uploadFile.name();
                _uploadFile.close();
                remove(filename.c_str());
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
    if (p->httpUploadStatus() == WiFiPortal::HTTPUploadStatus::Aborted) {
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
