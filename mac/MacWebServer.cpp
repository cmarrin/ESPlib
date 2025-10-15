/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "MacWebServer.h"

#include<fstream>
#include<iostream>
#include<sstream>
#include<arpa/inet.h>
#include<unistd.h>
#include <thread>

#include "WebFileSystem.h"

using namespace mil;
    
thread_local int _fdClient;

void
WebServer::parseRequest(const std::string& rawRequest, ArgMap& headers, ArgMap& args)
{
    std::string method;
    std::string path;
    
    int currindex = 0;
    while(currindex < rawRequest.length()) {
        if(rawRequest[currindex] == ' ') {
            break;
        }
        method += rawRequest[currindex];
        currindex++;
    }
    headers["method"] = method;

    currindex++;
    while(currindex < rawRequest.length()){
        if(rawRequest[currindex] == ' '){
            break;
        }
        path += rawRequest[currindex];
        currindex++;
    }
   
    headers["path"] = path;
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
WebServer::buildHTTPHeader(int statuscode, size_t contentLength, std::string mimetype, const ArgMap& extraHeaders)
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
WebServer::sendHTTPResponse(int code, const char* mimetype, const char* data, const ArgMap& extraHeaders)
{
    std::string body(data);
    std::string response = buildHTTPHeader(code, body.size(), mimetype, extraHeaders);
    response += body;
    write(_fdClient, response.c_str(), response.length());
}

void
WebServer::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip, const ArgMap& extraHeaders)
{
    ArgMap headers = extraHeaders;
    
    if (gzip) {
        headers["Content-Encoding"] = "gzip";
    }

    std::string response = buildHTTPHeader(code, length, mimetype, headers);
    write(_fdClient, response.c_str(), response.length());    
    write(_fdClient, data, length);    
}

void
WebServer::streamHTTPResponse(File& file, const char* mimetype, bool attach, const ArgMap& extraHeaders)
{
    ArgMap headers = extraHeaders;

    // For now assume this is a file download. So set Content-Disposition
    std::string disp = attach ? "attachment" : "inline";
    disp += "; filename=\"";
    disp += file.name();
    disp += "\"";
    
    headers["Content-Disposition"] = disp;

    std::string response = buildHTTPHeader(200, file.size(), mimetype, extraHeaders);
    write(_fdClient, response.c_str(), response.length());
    
    while (true) {
        uint8_t buf[1024];
        int size = file.read(buf, 1024);
        if (size < 0) {
            printf("**** Error reading file\n");
            sendHTTPResponse(404, "text/plain", "Error reading file");
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
        printf("File not found.\n");
        sendHTTPResponse(404);
    } else {
        File file = _wfs->open(f.c_str(), "r");
        streamHTTPResponse(file, WebFileSystem::suffixToMimeType(f).c_str(), false);
        file.close();
    }
}

void
WebServer::handleClient(int fdClient)
{
    // Thread local variable
    _fdClient = fdClient;
    
    char clientReqBuffer[1024];
    
    read(_fdClient, clientReqBuffer, 1024);
    
    HeaderMap headers;    
    parseRequest(clientReqBuffer, headers);
    std::string path = headers["path"];
    
    // If we have the root path, return index.html
    if (path.empty() || path == "/") {
        sendStaticFile("index.html", "/");
    } else {
        // Go through all the handlers
        // FIXME: For now the endpoint is the string up to the 
        // first slash.
        if (path[0] != '/') {
            path = "/" + path;
        }
        
        std::string endpoint;
        size_t slash = path.substr(1).find('/');
        if (slash == std::string::npos) {
            endpoint = path;
            path = "";
        } else {
            endpoint = path.substr(0, slash + 1);
            path = path.substr(slash + 2);
        }
        
        for (const auto& it : _handlers) {
            if (it.endpoint == endpoint) {
                if (it.isStatic) {
                    sendStaticFile(path.c_str(), it.path.c_str());
                } else {
                    // Let handler deal with it
                    if (it.requestCB) {
                        it.requestCB();
                    }
                }
                break;
            }
        }
    }
    close(_fdClient);
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
       
        // Create a new thread to handle the client
        std::thread clientThread([this, fdClient]() { handleClient(fdClient); });
        clientThread.detach();
    }
}

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
};
