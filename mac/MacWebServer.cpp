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

using namespace mil;

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
   
    if (_path.empty() || _path == "/") {
        _path = "/index.html";
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
HttpResponse::frameHttpResponse(std::string statuscode,
                                std::string statusmsg, 
                                std::map<std::string, std::string> headers, 
                                std::string body,
                                std::string mimetype)
{
    headers["content-type"] = mimetype;
    headers["content-length"] = std::to_string(body.length());
    std::ostringstream buffer;
    buffer << "HTTP/1.1 " << statuscode << " " << statusmsg << "\r\n";
    
    for(auto x : headers) {
        buffer << x.first << ": " << x.second << "\r\n";
    }
    
    buffer << "\r\n" << body;
    return buffer.str();
}

void
Server::handleClient(int fdClient)
{
    char clientReqBuffer[1024];
    
    read(fdClient, clientReqBuffer, 1024);
    mil::HttpRequest req;
    
    req.parseRequest(clientReqBuffer);
    std::string mimetype = req.getMimeType(req.path());
    
    mil::HttpResponse res;
    
    std::string body = req.readHtmlFile(req.path());
    std::string response = res.frameHttpResponse("200", "OK", req.headers(), body, mimetype);

    write(fdClient, response.c_str(), response.length());
    
    close(fdClient);
}

void
Server::handleServer(int fdServer)
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
Server::start(int port)
{
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
