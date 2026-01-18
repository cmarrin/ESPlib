/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#if defined ARDUINO
#include "LittleFS.h"
#else
#include "LittleFSShim.h"
#endif

#include "LuaManager.h"

// Web based filesystem. It can be integrated with WebServer
// so HTTP requests can be made to upload files, read existing
// files, create directories, delete files and directories and do
// directory listings.

namespace mil {

class Application;
class WiFiPortal;

class WebFileSystem
{
  public:
    bool begin(Application*, bool format);
    fs::File open(const char* path, const char* mode = "r", bool create = false);
    
    size_t totalBytes();
    size_t usedBytes();
    
    bool exists(const char* path);
    bool remove(const char* path);
    bool rename(const char* fromPath, const char* toPath);
    bool mkdir(const char* path);
    bool rmdir(const char* path);

    std::string realPath(const std::string& path) const;
    
    void sendLandingPage(WiFiPortal*);
    void sendWiFiPage(WiFiPortal*);

  private:
    bool prepareFile(WiFiPortal* p, std::string& path);
    std::string listDir(const char* dirname, uint8_t levels);
    
    void handleUpload(WiFiPortal*);
    
    fs::File _uploadFile;
    bool _uploadAborted = false;
    std::string _uploadFilename;
    
    LuaManager _lua;
};

}
