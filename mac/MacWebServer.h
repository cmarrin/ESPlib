/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// MacWebServer
//
// A simple web server for mac
//
// Adapted from https://github.com/Aryandev12/webby-http-server/
//

#pragma once

#include "WiFiPortal.h"
#include <map>
#include <string>

class File;

namespace mil {

class WebFileSystem;

class WebServer
{
public:
    using HandlerCB = std::function<void()>;
    using ArgMap = std::map<std::string, std::string>;
    
    static constexpr int UploadBufferReturnSize = 2048;
    
    // While reading upload data we need to check for the boundary string.
    // If we hit the UploadBufferReturnSize and we're in the middle of 
    // checking for the boundary string, we need to keep reading until
    // we get it or we fail to get it. In the former case we return the
    // data up to where the boundary starts and make that the last buffer.
    // In the latter case we need to send UploadBufferReturnSize then
    // put the rest of the data at the head of the buffer and continue
    // reading. 80 bytes is enough for the max boundary string size plus
    // a couple of \r\n and a couple of '-'. 
    static constexpr int UploadBufferActualSize = UploadBufferReturnSize + 80;

    WebServer() { }
    ~WebServer() { stop(); }

    int start(WebFileSystem* fs, int port);
    void stop() { }
    
    void process();
    
    int32_t addHTTPHandler(const char* endpoint, WiFiPortal::HTTPMethod method, HandlerCB requestCB, HandlerCB uploadCB)
    {
        _handlers.emplace_back(endpoint, "", requestCB, uploadCB, false);
        return int32_t(_handlers.size());
    }

    void serveStatic(const char* uri, const char* path)
    {
        _handlers.emplace_back(uri, path, nullptr, nullptr, true);
    }

    void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "", const ArgMap& extraHeaders = ArgMap());
    void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip, const ArgMap& extraHeaders = ArgMap());
    void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach, const ArgMap& extraHeaders = ArgMap());

    std::string getHTTPArg(const char* name) { return _argMap[name]; }
    
    WiFiPortal::HTTPUploadStatus httpUploadStatus() const { return _uploadStatus; }
    std::string httpUploadFilename() const { return _uploadFilename; }
    std::string httpUploadName() const { return _uploadName; }
    std::string httpUploadType() const { return _uploadMimetype; }
    size_t httpUploadTotalSize() const { return _uploadTotalSize; }
    size_t httpUploadCurrentSize() const { return _uploadCurrentSize; }
    const uint8_t* httpUploadBuffer() const { return _uploadBuffer; }

private:
    void sendErrorResponse(int code, const char* error);
    void handleClient(int fdClient);
    void handleServer(int fdServer);
    void handleUpload(int fd, const ArgMap& headers, HandlerCB requestCB, HandlerCB uploadCB);

    void sendStaticFile(const char* filename, const char* path);
    
    std::string buildHTTPHeader(int statuscode, size_t contentLength, std::string mimetype, const ArgMap& extraHeaders = ArgMap());
    
    struct HTTPHandler
    {
        std::string endpoint, path;
        HandlerCB requestCB, uploadCB;
        bool isStatic;
    };
    
    std::vector<HTTPHandler> _handlers;
    
    ArgMap _argMap;
    WiFiPortal::HTTPUploadStatus _uploadStatus = WiFiPortal::HTTPUploadStatus::None;
    std::string _uploadFilename;
    std::string _uploadName;
    std::string _uploadMimetype;
    size_t _uploadTotalSize = 0;
    size_t _uploadCurrentSize = 0;
    uint8_t _uploadBuffer[UploadBufferActualSize];
    
    int _fdClient = -1; // This is only valis during handleClient
    
    std::vector<int> _clientsToProcess;
    
    WebFileSystem* _wfs = nullptr;
};

}
