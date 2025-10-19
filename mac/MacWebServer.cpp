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
#include <regex>

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

// String split function
static std::vector<std::string> split(const std::string& str, char sep)
{
    std::vector<std::string> strings;
    
    int startIndex = 0;
    int endIndex = 0;
    
    for (int i = 0; i <= str.size(); i++) {
        if (str[i] == sep || i == str.size()) {
            endIndex = i;
            std::string temp;
            temp.append(str, startIndex, endIndex - startIndex);
            strings.push_back(temp);
            startIndex = endIndex + 1;
        }
    }
    return strings;
}

static std::string trimWhitespace(const std::string& s)
{
    std::string returnString(s);
    returnString.erase(0, returnString.find_first_not_of(" \t"));
    returnString.erase(returnString.find_last_not_of(" \t") + 1);
    return returnString;
}

static std::string removeQuotes(const std::string& s)
{            
    if (s[0] == '"') {
        std::string returnString = s.substr(1);
        returnString.pop_back();
        return returnString;
    }
    return s;
}

static std::string getLine(int fd)
{
    char buf[4096];
    
    // Get a line
    int index = 0;
    bool needNewLine = false;
    
    while (true) {
        ssize_t size = read(fd, buf + index, 1);
        if (size < 0) {
            perror("read failed");
            return "";
        }

        if (size != 1) {
            printf("Internal error in parseRequestHeader\n");
            return "";
        }
        
        if (needNewLine) {
            if (buf[index] == '\n') {
                buf[index - 1] = '\0';
                break;
            } else {
                printf("Expected newline in parseRequestHeader\n");
                return "";
            }
        }
        
        if (buf[index] == '\r') {
            needNewLine = true;
        }
        index++;
    }
    return buf;
}

// Key/value pairs is separated by ':'
static std::vector<std::string> parseKeyValue(const std::string& s)
{
    // Handle key:value pairs
    std::vector<std::string> line = split(s, ':');
    line[0] = trimWhitespace(line[0]);
    line[1] = trimWhitespace(line[1]);
    return line;
}

// FormData has this form:
//
//      <value> { ';' <key> '=' <value> }
//
// This can be used to parse lines in the header and at the 
// start of a multipart/form-data section of the body.
// Return value is a vector where [0] and [1] are the main
// key value pairs and pairs of values after that are any
// form data present
//
static std::vector<std::string> parseFormData(const std::string& value)
{
    std::vector<std::string> retVal;
    
    // The value can be further split on ';'
    std::vector<std::string> line = split(value, ';');
    line[0] = trimWhitespace(line[0]);
    retVal.push_back(line[0]);
    line.erase(line.begin());
    
    // If we have further key/value pairs, save them        
    for (int i = 0; i < line.size(); ++i) {
        std::vector<std::string> pair = split(line[i], '=');
        retVal.push_back(trimWhitespace(pair[0]));
        retVal.push_back(trimWhitespace(pair[1]));
    }
    
    return retVal;
}

// Function to parse query string into a map
static WebServer::ArgMap parseQuery(const std::string& query)
{
    WebServer::ArgMap result;
    std::regex paramRegex("([^&=]+)=([^&]*)");
    auto begin = std::sregex_iterator(query.begin() + 1, query.end(), paramRegex);
    auto end = std::sregex_iterator();
    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        result[match[1]] = match[2];
    }
    return result;
}

static bool parseRequestHeader(int fd, WebServer::ArgMap& headers, WebServer::ArgMap& args)
{
    bool isFirstLine = true;
    std::string path;
    
    while (true) {
        // Get a line
        std::string line = getLine(fd);
        
        // We have a line in the buffer
        // TODO: Do a bunch of error checking here
        if (line.empty()) {
            // Blank line, we're done
            break;
        }
        
        if (isFirstLine) {
            isFirstLine = false;
            
            // Handle first line
            std::vector<std::string> parsedLine = split(line, ' ');
            headers["method"] = parsedLine[0];
            path = parsedLine[1];
            headers["path"] = path;
        } else {
            std::vector<std::string> keyValue = parseKeyValue(line);
            headers[keyValue[0]] = keyValue[1];
        }
    }

    // Pick out args
    size_t firstArg = path.find('?');
    if (firstArg == std::string::npos) {
        return true;
    }
    
    std::string argList = path.substr(firstArg);
    headers["path"] = path.substr(0, firstArg);

    args = parseQuery(argList);
    
    return true;
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
    std::string response = buildHTTPHeader(code, body.size(), mimetype ?: "text/plain", extraHeaders);
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

    std::string response = buildHTTPHeader(200, file.size(), mimetype, headers);
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
WebServer::handleUpload(int fd, const ArgMap& headers, HandlerCB requestCB, HandlerCB uploadCB)
{
    // For now we handle ContentType: multipart. We accept exactly 2 parts. The first
    // is a key value pair which is placed in _argMap. The second is the uploaded file
    // data including the filename (which is placed in _argMap). The next line is Content-Type
    // followed by a blank line, followed by the uploaded file data
    std::string contentType = headers.at("Content-Type");
    if (contentType.empty()) {
        sendHTTPResponse(501);
        return;
    }
    
    std::vector<std::string> multipart = parseFormData(contentType);
    if (multipart[0] != "multipart/form-data" || multipart[1] != "boundary") {
        sendHTTPResponse(501);
        return;
    }

    std::string boundary = multipart[2];

    while (true) {
        // The next line should be the boundary line
        std::string line = getLine(fd);

        // Ignore the first 2 characters of the boundary
        if (line.substr(2) != boundary) {
            sendHTTPResponse(204);
            return;
        }
        
        // First line of each section is a Content-Disposition
        line = getLine(fd);
        multipart.clear();
        multipart = parseKeyValue(line);
        if (multipart[0] != "Content-Disposition") {
            sendHTTPResponse(204);
            return;
        }
        multipart = parseFormData(multipart[1]);
        if (multipart[0] != "form-data") {
            sendHTTPResponse(204);
            return;
        }
        
        // Next should be a "name" key/value pair
        if (multipart[1] != "name") {
            sendHTTPResponse(204);
            return;
        }
        
        // The key might have quotes around it
        std::string key = removeQuotes(multipart[2]);

        // The value of the "name" pair can either be a query param or "files[]"
        // If it's the latter then there should be a "filename" key/value pair
        if (key != "files[]") {
            // This is a query param. The key is in multipart[2]. The value is
            // content, which comes after a blank line
            line = getLine(fd);
            if (line[0] != '\0') {
                sendHTTPResponse(204);
                return;
            }
            
            line = getLine(fd);
            _argMap[key] = line;
        } else {
            if (multipart[3] != "filename") {
                sendHTTPResponse(204);
                return;
            }
            
            // We're at the content. But first the next line should be "Content-Type"
            line = getLine(fd);
            multipart = parseKeyValue(line);
            if (multipart[0] != "Content-Type") {
                sendHTTPResponse(204);
                return;
            }

            _uploadMimetype = multipart[1];
            
            // Now let's get to the content, the next line should be blank
            line = getLine(fd);
            if (line[0] != '\0') {
                sendHTTPResponse(204);
                return;
            }
            
            // Now we upload
            if (uploadCB) {
                // We need to add "\n--" to the start of the boundary
                boundary = "\n--" + boundary;

                // Start upload
                _uploadName = "";
                _uploadStatus = WiFiPortal::HTTPUploadStatus::Start;
                uploadCB();
                
                int bufferOverflow = 0;
                int index = 0;
                _uploadTotalSize = 0;
                
                while (true) {
                    // We need to read one byte at a time.
                    // As we go we need to check for the boundary string while
                    // we keep accumulating data in the buffer. If we do hit
                    // the boundary string we need to return the buffer data
                    // up to the start of the string and this is the last buffer.
                    // If it fails we need to send UploadBufferReturnSize bytes
                    // then put the remaining data at the head of the buffer
                    // and continue reading. The actual size of the buffer
                    // (UploadBufferActualSize) is big enough to hold the extra
                    // bytes while we're checking for the boundary string.
                    //
                    _uploadStatus = WiFiPortal::HTTPUploadStatus::Write;

                    ssize_t size = read(fd, &(_uploadBuffer[index]), 1);
                    if (size != 1) {
                        sendHTTPResponse(204);
                        return;
                    }

                    bool haveBoundary = false;
                    if (_uploadBuffer[index] == '\r') {
                        haveBoundary = true;
                        ++index;
                        // This might be the boundary
                         // We start at -3 because we need to check the \n-- at the start of the boundary
                        for (int i = 0; i < boundary.size(); ++i, ++index) {
                            size = read(fd, &(_uploadBuffer[index]), 1);
                            if (size != 1) {
                                sendHTTPResponse(204);
                                return;
                            }
                        
                            if (_uploadBuffer[index] != boundary[i]) {
                                // This is not the boundary
                                haveBoundary = false;
                                ++index;
                                if (index > UploadBufferReturnSize) {
                                    // We are in the overflow area. Set the buffer
                                    // size to UploadBufferReturnSize. After sending
                                    // the data move the overflow to the start of the
                                    // buffer and continue
                                    bufferOverflow = index - UploadBufferReturnSize;;
                                    index = UploadBufferReturnSize;
                                }
                                break;
                            }
                        }
                    }
                    
                    ++index;
                    assert(index <= UploadBufferReturnSize);
                    
                    if (index == UploadBufferReturnSize || haveBoundary) {
                        // Send the buffer
                        _uploadCurrentSize = haveBoundary ? (index - boundary.size() - 2) : UploadBufferReturnSize;
                        _uploadTotalSize += _uploadCurrentSize;
                        uploadCB();
                        index = 0;
                        
                        // Deal with any overflow
                        if (bufferOverflow > 0) {
                            memcpy(_uploadBuffer, _uploadBuffer + UploadBufferReturnSize, bufferOverflow);
                            index = bufferOverflow;
                            bufferOverflow = 0;
                        }
                    
                        // Break when we've sent all the data
                        if (haveBoundary) {
                            break;
                        }
                    }
                }
            
                _uploadStatus = WiFiPortal::HTTPUploadStatus::End;
                uploadCB();
                
                if (requestCB) {
                    requestCB();
                }
            }
        }
    }
}

void
WebServer::handleClient(int fdClient)
{
    // Set the member variable, only valid for the duration of this call
    _fdClient = fdClient;

    ArgMap headers;
    _argMap.clear();

    parseRequestHeader(fdClient, headers, _argMap);
    
    std::string path = headers["path"];
    bool isUpload = headers["method"] == "POST";
    
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
                    if (isUpload) {
                        handleUpload(fdClient, headers, it.requestCB, it.uploadCB);
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
