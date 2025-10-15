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

    WebServer() { }
    ~WebServer() { stop(); }

    int start(WebFileSystem* fs, int port);
    void stop() { }
    
    int32_t addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB)
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
    void streamHTTPResponse(File& file, const char* mimetype, bool attach, const ArgMap& extraHeaders = ArgMap());

    std::string getHTTPArg(const char* name) { return _argMap[name]; }
    
private:
    void handleClient(int fdClient);
    void handleServer(int fdServer);

    void sendStaticFile(const char* filename, const char* path);
    
    void parseRequest(const std::string& rawRequest, ArgMap& headers, ArgMap& args);
    std::string buildHTTPHeader(int statuscode, size_t contentLength, std::string mimetype, const ArgMap& extraHeaders = ArgMap());
    
    struct HTTPHandler
    {
        std::string endpoint, path;
        HandlerCB requestCB, uploadCB;
        bool isStatic;
    };
    
    std::vector<HTTPHandler> _handlers;
    
    ArgMap _argMap;

    WebFileSystem* _wfs = nullptr;
};

}
