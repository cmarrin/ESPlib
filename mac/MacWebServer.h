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
    public:
    std::string frameHttpResponse(std::string statuscode,
                                  std::string statusmsg, 
                                  std::map<std::string, std::string> headers,
                                  std::string body,
                                  std::string mimetype);
};

class Server
{
public:
    Server(std::string port) : _port(port) { }
    ~Server() { stop(); }

    int start();
    void stop() { }
    
private:
    std::string _port;
};

}
