/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// This code was adapted from:
//
//      https://github.com/FooBarWidget/multipart-parser
//
// It is heavily edited to bring it up to modern C++ standards and 
// to make it prettier :-)
//
// In its original form it simply parsed and sent back the component parts.
// The caller need to have state to complete the parse. I've added the ability
// to add the header fields to a map that the caller can use to determine
// what to do with this part. So now each part gets a callback with the start
// of the data, another for each chunk, and one more at the end. The parser
// keeps the header map which can be queried as needed.
//
// Since the main header fields are the same format, this parser can be used
// to parse those as well.

#pragma once

#include <sys/types.h>
#include <string>
#include <array>
#include <functional>
#include <map>

#include "WiFiPortal.h"

namespace mil {

class HTTPParser
{
  public:
    static constexpr int UploadBufferReturnSize = 8192;
    
    // While reading upload data we need to check for the boundary string.
    // If we hit the UploadBufferReturnSize and we're in the middle of 
    // checking for the boundary string, we need to keep reading until
    // we get it or we fail to get it. In the former case we return the
    // data up to where the boundary starts and make that the last buffer.
    // In the latter case we need to send UploadBufferReturnSize then
    // put the rest of the data at the head of the buffer and continue
    // reading. 80 bytes is enough for the max boundary string size plus
    // a couple of \r\n and a couple of '-'. 
    static constexpr int UploadBufferActualSize = UploadBufferReturnSize + 80;

    using ReadCB = std::function<ssize_t(uint8_t* buf, size_t size)>;
    using HandlerCB = std::function<void()>;
    
    using ArgMap = std::map<std::string, std::string>;

	HTTPParser()
    {
        _uploadBuffer = new uint8_t[UploadBufferActualSize];
    }
    
	~HTTPParser()
    {
        delete [ ] _uploadBuffer;
    }

    bool parseMultipart(size_t size, const std::string& boundary, HandlerCB, ReadCB);
    
    bool parseRequestHeader(ReadCB cb);
    void parseQuery(const std::string& query);
    
    static std::string getLine(ReadCB cb);
    static std::string urlDecode(const std::string&);
    static std::string suffixToMimeType(const std::string& filename);
    static std::vector<std::string> split(const std::string& str, char sep);
    static std::string trimWhitespace(const std::string& s);
    static std::string removeQuotes(const std::string& s);
    static std::vector<std::string> parseFormData(const std::string& value);
    
    void setErrorResponse(int code, const char* error)
    {
        _errorCode = code;
        _errorReason = error;
    }
    
    int errorCode() const { return _errorCode; }
    const std::string& errorReason() const { return _errorReason; }
    
    const std::string& method() const { return _method; }
    const std::string& path() const { return _path; }
    const std::string getHTTPArg(const char* name) { return _args[name]; }
    const std::string getHeader(const char* name) { return _headers[name]; }
    
    WiFiPortal::HTTPUploadStatus httpUploadStatus() const { return _uploadStatus; }
    std::string httpUploadFilename() const { return _uploadFilename; }
    size_t httpUploadContentLength() const { return _uploadContentLength; }
    size_t httpUploadTotalSize() const { return _uploadTotalSize; }
    size_t httpUploadCurrentSize() const { return _uploadCurrentSize; }
    const uint8_t* httpUploadBuffer() const { return _uploadBuffer; }

private:
    static std::vector<std::string> parseKeyValue(const std::string& s);

    HTTPParser::ArgMap _args;
    HTTPParser::ArgMap _headers;
    std::string _method;
    std::string _path;
    
    int _errorCode = 0;
    std::string _errorReason;
    
    // For upload
    WiFiPortal::HTTPUploadStatus _uploadStatus = WiFiPortal::HTTPUploadStatus::None;
    size_t _uploadContentLength = 0;
    std::string _uploadFilename;
    std::string _uploadMimetype;
    size_t _uploadTotalSize = 0;
    size_t _uploadCurrentSize = 0;
    uint8_t* _uploadBuffer = nullptr;
};

}
