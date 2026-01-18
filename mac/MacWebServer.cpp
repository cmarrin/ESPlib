/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "MacWebServer.h"

#include<cassert>
#include<fstream>
#include<iostream>
#include<sstream>
#include<arpa/inet.h>
#include<unistd.h>
#include <thread>

#include "WebFileSystem.h"

using namespace mil;
    
int
WebServer::start(WebFileSystem* wfs, int port)
{
    _wfs = wfs;
    
    int fdServer = socket(AF_INET, SOCK_STREAM, 0);
    if (fdServer < 0) {
        printf("Failed to create server socket.\n");
        return -1;
    }

    int opt = 1;
    if (setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("setsockopt(SO_REUSEADDR) failed\n");
        return -1;
    }
    
    struct sockaddr_in serverAddr;
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind  socket to port.
    if (bind(fdServer, (struct sockaddr*)& serverAddr, sizeof(serverAddr)) < 0) {
        printf("Failed to bind server socket.\n");
        return -1;
    }

    //listens on socket.
    if (listen(fdServer, 5) < 0) {
        printf("Failed to listen on server socket.\n");
        return -1;
    }
    
    printf("Server started on port : %d\n", port);

    std::thread serverThread([this, fdServer]() { handleServer(fdServer); });
    serverThread.detach();
    
    return fdServer;
}

void
WebServer::process()
{
    // Handle one client at a time so we don't overload
    if (!_clientsToProcess.empty()) {
        handleClient(_clientsToProcess[0]);
        _clientsToProcess.erase(_clientsToProcess.begin());
    }
}

static const char* responseCodeToString(int code)
{
    switch (code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Time-out";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Large";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested range not satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Time-out";
        case 505: return "HTTP Version not supported";
        default:  return "";
    }
}

std::string
WebServer::buildHTTPHeader(int statuscode, size_t contentLength, std::string mimetype, const HTTPParser::ArgMap& extraHeaders)
{
    std::ostringstream buffer;
    buffer << "HTTP/1.1 " << statuscode << " " << responseCodeToString(statuscode) << "\r\n";
    buffer << "content-type" << ": " << mimetype << "\r\n";
    buffer << "content-length" << ": " << std::to_string(contentLength) << "\r\n";
    
    for (const auto& it : extraHeaders) {
        buffer << it.first << ": " << it.second << "\r\n";
    }
    
    buffer << "\r\n";
    return buffer.str();
}

void
WebServer::sendHTTPResponse(int code, const char* mimetype, const char* data, const HTTPParser::ArgMap& extraHeaders)
{
    if (code != 200) {
        printf("Error Response code (%d): %s\n", code, responseCodeToString(code));
    }
    
    std::string body(data);
    std::string response = buildHTTPHeader(code, body.size(), mimetype ?: "text/plain", extraHeaders);
    response += body;
    write(_fdClient, response.c_str(), response.length());
}

void
WebServer::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip, const HTTPParser::ArgMap& extraHeaders)
{
    HTTPParser::ArgMap headers = extraHeaders;
    
    if (gzip) {
        headers["Content-Encoding"] = "gzip";
    }

    std::string response = buildHTTPHeader(code, length, mimetype, headers);
    write(_fdClient, response.c_str(), response.length());    
    write(_fdClient, data, length);    
}

void
WebServer::streamHTTPResponse(fs::File& file, const char* mimetype, bool attach, const HTTPParser::ArgMap& extraHeaders)
{
    HTTPParser::ArgMap headers = extraHeaders;

    // For now assume this is a file download. So set Content-Disposition
    std::string disp = attach ? "attachment" : "inline";
    disp += "; filename=\"";
    disp += file.name();
    disp += "\"";
    
    headers["Content-Disposition"] = disp;

    std::string response = buildHTTPHeader(200, file.size(), mimetype, headers);
    write(_fdClient, response.c_str(), response.length());
    
    while (true) {
        uint8_t buf[1024];
        int size = file.read(buf, 1024);
        if (size < 0) {
            printf("**** Error reading file\n");
            if (_parser) {
                _parser->setErrorResponse(404, "Error reading file");
            }
            break;
        }
        
        write(_fdClient, buf, size);
        if (size < 1024) {
            break;
        }
    }
}

void
WebServer::sendStaticFile(const char* filename, const char* path)
{
    std::string f(path);
    f += filename;
    
    if (!_wfs || !_wfs->exists(f.c_str())) {
       if (_parser) {
            _parser->setErrorResponse(404, "File not found");
        }
    } else {
        fs::File file = _wfs->open(f.c_str(), "r");
        streamHTTPResponse(file, HTTPParser::suffixToMimeType(f).c_str(), false);
        file.close();
    }
}

void
WebServer::handleClient(int fdClient)
{
    // Set the member variable, only valid for the duration of this call
    _fdClient = fdClient;

    if (_parser) {
        printf("***** Already in the middle of a request\n");
        _parser.release();
        return;
    }
    
    _parser = std::make_unique<HTTPParser>();
    
    _parser->parseRequestHeader([fdClient](uint8_t* buf, size_t size) -> ssize_t {
        return read(fdClient, buf, size);
    });
    
    bool isUpload = _parser->method() == "POST";
    std::string filePath = _parser->path();
    
    // If we have the root path, return index.html
    if (filePath.empty() || filePath == "/") {
        sendStaticFile("index.html", "/");
    } else {
        // Go through all the handlers
        // FIXME: For now the endpoint is the string up to the 
        // first slash.
        if (filePath[0] != '/') {
            filePath = "/" + filePath;
        }
        
        std::string endpoint;
        size_t slash = filePath.substr(1).find('/');
        if (slash == std::string::npos) {
            endpoint = filePath;
            filePath = "";
        } else {
            endpoint = filePath.substr(0, slash + 1);
            filePath = filePath.substr(slash + 2);
        }
        
        for (const auto& it : _handlers) {
            if (it.endpoint == endpoint) {
                if (it.isStatic) {
                    sendStaticFile(filePath.c_str(), it.path.c_str());
                } else {
                    if (isUpload) {
                        _parser->parseMultipart(it.requestCB, [fdClient](uint8_t* buf, size_t size) -> ssize_t
                        {
                            return read(fdClient, buf, size);
                        });
                    } else {
                        if (it.requestCB) {
                            it.requestCB();
                        }
                    }
                }
                break;
            }
        }
    }
    close(_fdClient);
    _fdClient = -1;
    _parser.release();
}

void
WebServer::handleServer(int fdServer)
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(struct sockaddr_in); 

    while (1) {
        int fdClient = accept(fdServer, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (fdClient < 0) {
            printf("Failed to accept client request.\n");
            return;
        }
        // process() will process the client
        _clientsToProcess.push_back(fdClient);
    }
}
