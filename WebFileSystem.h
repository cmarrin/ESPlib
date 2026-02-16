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
    static fs::File open(const char* path, const char* mode = "r", bool create = false);
    
    static size_t totalBytes();
    static size_t usedBytes();
    
    static bool exists(const char* path);
    static bool remove(const char* path);
    static bool rename(const char* fromPath, const char* toPath);
    static bool mkdir(const char* path);
    static bool rmdir(const char* path);

    static std::string realPath(const std::string& path);
    
    void sendLandingPage(WiFiPortal*);
    void sendWiFiPage(WiFiPortal*);

    static inline std::string quote(const std::string& s) { return "\"" + s + "\""; }
    static inline std::string jsonParam(const std::string& n, const std::string& v) { return quote(n) + ":" + quote(v); }
    static std::string makeJSON(std::vector<std::pair<std::string, std::string>> json);
    
  private:
    bool prepareFile(WiFiPortal* p, std::string& path);
    std::string listDir(const char* dirname, uint8_t levels);
    
    void handleUpload(WiFiPortal*);
    void handleLandingSetup(WiFiPortal*);

    fs::File _uploadFile;
    bool _uploadAborted = false;
    std::string _uploadFilename;
};

}
