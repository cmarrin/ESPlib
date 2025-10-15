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
HttpRequest::parseRequest(const std::string& rawRequest)
{
    int currindex=0;
    while(currindex < rawRequest.length()) {
        if(rawRequest[currindex] == ' ') {
            break;
        }
        _method += rawRequest[currindex];
        currindex++;
    }
    _headers["method"] = _method;

    currindex++;
    while(currindex < rawRequest.length()){
        if(rawRequest[currindex] == ' '){
            break;
        }
        _path += rawRequest[currindex];
        currindex++;
    }
   
    _headers["path"] = _path;
}

std::string
HttpRequest::readHtmlFile(const std::string &path)
{
    std::string filename = path.substr(1,path.length());
   
    std::ifstream file(filename);
    int flag = 0;
    if (!file) {
        flag=1;
        printf("File not found.\n");
    }

    if (flag == 0) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    return "";
}

std::string
HttpRequest::getMimeType(const std::string &path)
{
    std::map<std::string, std::string> mimeTypes = {
        {"html","text/html"},
        {"css","text/css"},
        {"js","text/javascript"},
        {"jpg","image/jpeg"},
        {"png","image/png"}
    };
    
    std::string fileExtension = path.substr(path.find_last_of(".") + 1);
    return mimeTypes[fileExtension];
}

std::string
WebServer::buildHTTPHeader(int statuscode, std::string statusmsg, size_t contentLength, std::string mimetype)
{
    std::ostringstream buffer;
    buffer << "HTTP/1.1 " << statuscode << " " << statusmsg << "\r\n";
    buffer << "content-type" << ": " << mimetype << "\r\n";
    buffer << "content-length" << ": " << std::to_string(contentLength) << "\r\n";    
    buffer << "\r\n";
    return buffer.str();
}

void
WebServer::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
    mil::HttpResponse res;
    
    std::string body(data);
    std::string response = buildHTTPHeader(200, "OK", body.size(), "text/html");
    response += body;
    write(_fdClient, response.c_str(), response.length());
}

void
WebServer::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
}

void
WebServer::streamHTTPResponse(File& file, const char* mimetype, bool attach)
{
    std::string response = buildHTTPHeader(200, "OK", file.size(), mimetype);
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
    mil::HttpRequest req;
    
    req.parseRequest(clientReqBuffer);
    
    // If we have the root path, return index.html
    if (req.path().empty() || req.path() == "/") {
        sendStaticFile("index.html", "/");
    } else {
        // Go through all the handlers
        // FIXME: For now the endpoint is the string up to the 
        // first slash.
        std::string path = req.path();
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
