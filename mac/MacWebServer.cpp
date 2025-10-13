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
   
    _headers["path"] = _path;
}

std::string
HttpRequest::readHtmlFile(const std::string &path)
{
    std::string filename = path.substr(1,path.length());
    if (filename.empty()) {
        filename = "index.html";
    }
   
    std::ifstream file(filename); //ifstream used for reading file if exists.
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

int
Server::start()
{
    int server_socket_fd;
    struct sockaddr_in server_addr;
    
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        printf("Failed to create server socket.\n");
        return -1;
    }

    //config server socket.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(std::stoi(_port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind  socket to port.
    if (bind(server_socket_fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0) {
        printf("Failed to bind server socket.\n");
        return -1;
    }

    //listens on socket.
    if (listen(server_socket_fd, 5) < 0) {
        printf("Failed to listen on server socket.\n");
        return -1;
    }
    
    printf("Server started on port : %s\n", _port.c_str());

    return server_socket_fd;
};
