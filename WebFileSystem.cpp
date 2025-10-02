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
        printf("Failed to open directory");
        return "";
    }
    
    if (!root.isDirectory()){
        printf("Not a directory");
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

    app->addHTTPHandler("/fs", [this](WiFiPortal* p, WiFiPortal::HTTPMethod m, const std::string& uri) -> bool
    {
        p->sendHTTPResponse(200, "text/html", reinterpret_cast<const char*>(FILEMGR_NAME), FILEMGR_LEN_NAME, FILEMGR_IS_GZIP);
        return true;
    });

    app->addHTTPHandler("/get-folder-contents", [this](WiFiPortal* p, WiFiPortal::HTTPMethod m, const std::string& uri) -> bool
    {
        std::string s = listDir(p->getHTTPArg("path").c_str(), 0);
        p->sendHTTPResponse(200, "text/html", s.c_str());
        return true;
    });

    app->addHTTPHandler("/upload", [this](WiFiPortal* p, WiFiPortal::HTTPMethod m, const std::string& uri) -> bool
    {
        if (m == WiFiPortal::HTTPMethod::Post) {
            std::string filename = p->httpUploadFilename();
            
            // Prepend a '/' if it doesn't have one
            if (filename[0] != '/') {
                filename = "/" + filename;
            }
            
            size_t size = p->httpUploadLength();
            uint8_t* buf = new uint8_t[size + 1];
            p->readHTTPContent(buf, size);
 
            printf("Uploading file '%s', size=%u...\n", filename.c_str(), (unsigned int) size);
            File f = open(filename.c_str(), "w");
            if (!f) {
                printf("***** failed to open '%s' for write\n", filename.c_str());
            } else {
                size_t r = f.write(buf, size);
                if (r != size) {
                    printf("***** failed to write '%s', error=%u\n", filename.c_str(), (unsigned int) r);
                } else {
                    printf("    upload complete.\n");
                }
            }
            f.close();

            delete [ ] buf;
            printf("Received file '%s'\n", uri.c_str());
            
            p->sendHTTPResponse(200, "application/json", "{\"status\":\"success\",\"message\":\"File upload complete\"}");
            return true;
        }
         return false;
    });

    return LittleFS.begin(format);
}
    
File
WebFileSystem::open(const char* path, const char* mode, bool create)
{
    return LittleFS.open(path, mode);
}
