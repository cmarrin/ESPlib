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
    File open(const char* path, const char* mode = "r", bool create = false);
    
  private:
    std::string listDir(const char* dirname, uint8_t levels);
    
    void handleUpload(WiFiPortal*);
    void handleUploadFinished(WiFiPortal*);
    
    File _uploadFile;
    bool _uploadAborted = false;
    std::string _uploadFilename;
};

}
