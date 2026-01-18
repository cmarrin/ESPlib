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
#include "HTTPParser.h"

#include <string>

class File;

namespace mil {

class WebFileSystem;

class WebServer
{
public:
    WebServer() { }
    ~WebServer() { stop(); }

    int start(WebFileSystem* fs, int port);
    void stop() { }
    
    void process();
    
    int32_t addHTTPHandler(const char* endpoint, WiFiPortal::HTTPMethod method, HTTPParser::HandlerCB requestCB)
    {
        _handlers.emplace_back(endpoint, "", requestCB, false);
        return int32_t(_handlers.size());
    }

    void serveStatic(const char* uri, const char* path)
    {
        _handlers.emplace_back(uri, path, nullptr, true);
    }

    void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "", const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());
    void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip, const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());
    void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach, const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());

    std::string getHTTPArg(const char* name) { return _parser ? _parser->getHTTPArg(name) : ""; }
    WiFiPortal::HTTPUploadStatus httpUploadStatus() const { return _parser ? _parser->httpUploadStatus() : WiFiPortal::HTTPUploadStatus::None; }
    std::string httpUploadFilename() const { return _parser ? _parser->httpUploadFilename() : ""; }
    size_t httpUploadTotalSize() const { return _parser ? _parser->httpUploadTotalSize() : 0; }
    size_t httpUploadCurrentSize() const { return _parser ? _parser->httpUploadCurrentSize() : 0; }
    const uint8_t* httpUploadBuffer() const { return _parser ? _parser->httpUploadBuffer() : nullptr; }

private:
    void handleClient(int fdClient);
    void handleServer(int fdServer);

    void sendStaticFile(const char* filename, const char* path);
    
    std::string buildHTTPHeader(int statuscode, size_t contentLength, std::string mimetype, const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());
    
    struct HTTPHandler
    {
        std::string endpoint, path;
        HTTPParser::HandlerCB requestCB;
        bool isStatic;
    };
    
    std::vector<HTTPHandler> _handlers;
        
    int _fdClient = -1; // This is only valis during handleClient
    
    std::vector<int> _clientsToProcess;
    
    WebFileSystem* _wfs = nullptr;
    
    std::unique_ptr<HTTPParser> _parser;
};

}
