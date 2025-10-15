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

class HttpRequest
{
public:
    void parseRequest(const std::string& rawRequest);
  
    std::string readHtmlFile(const std::string &path);
    std::string getMimeType(const std::string &path);

    const std::string& path() const { return _path; }
    const std::map<std::string, std::string>& headers() const { return _headers; }
    
private:
    std::string _method;
    std:: string _path;
    std::map<std::string, std::string> _headers;
};

class HttpResponse{ 
    std::string statuscode;
    std::string statusmsg;
    std::map<std::string, std::string> headers;
    std::string body;
};

class WebFileSystem;

class WebServer
{
public:
    using HandlerCB = std::function<void()>;

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

    void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "");
    void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip);
    void streamHTTPResponse(File& file, const char* mimetype, bool attach);
    
private:
    void handleClient(int fdClient);
    void handleServer(int fdServer);

    void sendStaticFile(const char* filename, const char* path);
    
    std::string buildHTTPHeader(int statuscode, std::string statusmsg, size_t contentLength, std::string mimetype);
    
    struct HTTPHandler
    {
        std::string endpoint, path;
        HandlerCB requestCB, uploadCB;
        bool isStatic;
    };
    
    std::vector<HTTPHandler> _handlers;

    WebFileSystem* _wfs = nullptr;
};

}
