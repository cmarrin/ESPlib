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

namespace fs {
    class File;
}

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
        // We handle only a very simple wildcard type. If the endpoint ends with "/*" then we
        // strip it off and set the type to Wildcard
        size_t len = strlen(endpoint);
        HTTPHandler::EndpointType type = HTTPHandler::EndpointType::Fixed;
        
        if (len > 2) {
            if (endpoint[len - 2] == '/' && endpoint[len - 1] == '*') {
                type = HTTPHandler::EndpointType::Wildcard;
                len -= 2;
            }
        }
        _handlers.emplace_back(std::string(endpoint, len).c_str(), "", requestCB, type);
        return int32_t(_handlers.size());
    }

    void addStaticHTTPHandler(const char* uri, const char* path)
    {
        _handlers.emplace_back(uri, path, nullptr, HTTPHandler::EndpointType::Static);
    }

    void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "", const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());
    void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip, const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());
    void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach, const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());

    std::string getHTTPArg(const char* name) { return _parser ? _parser->getHTTPArg(name) : ""; }
    void parseQuery(const char* queryString) { if (_parser) _parser->parseQuery(queryString); }
    std::string getHTTPHeader(const char* name) { return _parser ? _parser->getHTTPHeader(name) : ""; }
    WiFiPortal::HTTPUploadStatus httpUploadStatus() const { return _parser ? _parser->httpUploadStatus() : WiFiPortal::HTTPUploadStatus::None; }
    std::string httpUploadFilename() const { return _parser ? _parser->httpUploadFilename() : ""; }
    size_t httpUploadTotalSize() const { return _parser ? _parser->httpUploadTotalSize() : 0; }
    size_t httpUploadCurrentSize() const { return _parser ? _parser->httpUploadCurrentSize() : 0; }
    const uint8_t* httpUploadBuffer() const { return _parser ? _parser->httpUploadBuffer() : nullptr; }

    int receiveHTTPResponse(char* buf, size_t size) { return int(read(_fdClient, buf, size)); }
    
private:
    void handleClient(int fdClient);
    void handleServer(int fdServer);

    void sendStaticFile(const char* filename, const char* path);
    
    std::string buildHTTPHeader(int statuscode, size_t contentLength, const char* mimetype, const HTTPParser::ArgMap& extraHeaders = HTTPParser::ArgMap());
    
    struct HTTPHandler
    {
        enum class EndpointType { Fixed, Static, Wildcard };
        std::string endpoint, path;
        HTTPParser::HandlerCB requestCB;
        EndpointType type;
    };
    
    std::vector<HTTPHandler> _handlers;
        
    int _fdClient = -1; // This is only valis during handleClient
    
    std::vector<int> _clientsToProcess;
    
    WebFileSystem* _wfs = nullptr;
    
    std::unique_ptr<HTTPParser> _parser;
};

}
