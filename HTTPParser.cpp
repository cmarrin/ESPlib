/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "HTTPParser.h"

#include <cstring>
#include <filesystem>
#include <regex>

using namespace mil;

bool
HTTPParser::parseMultipart(const std::string& boundary, HandlerCB requestCB, ReadCB readCB)
{
    // For now we handle ContentType: multipart. We accept exactly 2 parts. The first
    // is a key value pair which is placed in _argMap. The second is the uploaded file
    // data including the filename (which is placed in _argMap). The next line is Content-Type
    // followed by a blank line, followed by the uploaded file data

    bool nextLineIsBoundary = true;
    bool done = false;
    
    while (!done) {
        std::string line;
        
        // If we just uploaded a file then we already read the boundary following it.
        // Otherwise the next line is a boundary
        if (nextLineIsBoundary) {
            line = getLine(readCB);

            // Ignore the first 2 characters of the boundary
            if (line.length() < 2 || line.substr(2) != boundary) {
                setErrorResponse(400, "missing boundary");
                return false;
            }
        } else {
            // For next time. If we upload a file then we set this to false
            nextLineIsBoundary = true;
        }
        
        // First line of each section is a Content-Disposition
        line = getLine(readCB);

        std::vector<std::string> parsedValue = parseKeyValue(line);
        
        if (parsedValue.size() < 2 || parsedValue[0] != "Content-Disposition") {
            setErrorResponse(400, "missing Content-Disposition");
            return false;
        }
        
        parsedValue = parseFormData(parsedValue[1]);
        if (parsedValue.size() < 1 || parsedValue[0] != "form-data") {
            setErrorResponse(400, "missing form-data");
            return false;
        }
        
        // Next should be a "name" key/value pair
        if (parsedValue.size() < 3 || parsedValue[1] != "name") {
            setErrorResponse(400, "missing name");
            return false;
        }
        
        // The key might have quotes around it
        std::string key = removeQuotes(parsedValue[2]);

        // The value of the "name" pair can either be a query param or a file upload.
        // If it's the latter then there should be a "filename" key/value pair and
        // we don't care about the value of the name.
        if (parsedValue.size() < 3) {
            setErrorResponse(400, "bad name format");
            return false;
        }
        
        if (parsedValue.size() < 5) {
            // This is a query param. The key is in multipart[2]. The value is
            // content, which comes after a blank line
            line = getLine(readCB);
            if (line[0] != '\0') {
                setErrorResponse(400, "should be blank line");
                return false;
            }
            
            line = getLine(readCB);
            _args[key] = line;
        } else {
            if (parsedValue[3] != "filename") {
                setErrorResponse(400, "missing filename");
                return false;
            }
            
            // This is a file upload section. That means we'll be parsing the
            // boundary following it here, so we don't need to do it above
            nextLineIsBoundary = false;

            // Set the filename
            _uploadFilename = removeQuotes(parsedValue[4]);
                
            // We're at the content. But first the next line should be "Content-Type"
            line = getLine(readCB);
            parsedValue = parseKeyValue(line);
            if (parsedValue.size() < 2 || parsedValue[0] != "Content-Type") {
                setErrorResponse(400, "missing Content-Type");
                return false;
            }

            _uploadMimetype = parsedValue[1];
            
           // Now let's get to the content, the next line should be blank
            line = getLine(readCB);
            if (line[0] != '\0') {
                setErrorResponse(400, "line should be blank");
                return false;
            }
            
            // Now we upload. If there is no requestCB we still need to go through
            // reading all the data

            // Start upload
            _uploadStatus = WiFiPortal::HTTPUploadStatus::Start;
            if (requestCB) {
                requestCB();
            }
            
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

                ssize_t size = readCB(_uploadBuffer + index, 1);
                if (size != 1) {
                    setErrorResponse(400, "read error");
                    return false;
                }

                bool haveBoundary = false;
                if (_uploadBuffer[index] == '\r') {
                    // We need to add "\n--" to the start of the boundary
                    std::string testBoundary = "\n--" + boundary;

                    haveBoundary = true;
                    ++index;
                    // This might be the boundary
                     // We start at -3 because we need to check the \n-- at the start of the boundary
                    for (int i = 0; i < testBoundary.size(); ++i, ++index) {
                        size = readCB(_uploadBuffer +index, 1);
                        if (size != 1) {
                            setErrorResponse(400, "read error");
                            return false;
                        }
                    
                        if (_uploadBuffer[index] != testBoundary[i]) {
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
                    
                    if (haveBoundary) {
                        // After the boundary there should be a --\r\n if
                        // we're at the last file or \r\n if not
                        uint8_t endBuf[2];
                        size = readCB(endBuf, 2);
                        if (size != 2) {
                            setErrorResponse(400, "read error");
                            return false;
                        }
                        if (endBuf[0] == '-' && endBuf[1] == '-') {
                            // We're done
                            done = true;
                            size = readCB(endBuf, 2);
                        }
                        if (endBuf[0] != '\r' || endBuf[1] != '\n') {
                            setErrorResponse(400, "missing end of line");
                            return false;
                        }
                    }
                } else {
                    ++index;
                }
                
                if (index > UploadBufferReturnSize) {
                    setErrorResponse(404, "Internal error");
                    return false;
                }
                
                if (index == UploadBufferReturnSize || haveBoundary) {
                    // Send the buffer
                    _uploadCurrentSize = haveBoundary ? (index - boundary.size() - 4) : UploadBufferReturnSize;
                    _uploadTotalSize += _uploadCurrentSize;
                    if (requestCB) {
                        requestCB();
                    }
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
            if (requestCB) {
                requestCB();
            }
        }
    }
    return true;
}

std::string
HTTPParser::getLine(ReadCB cb)
{
    std::string s;
    
    // Get a line
    bool needNewLine = false;
    
    while (true) {
        uint8_t c;
        ssize_t size = cb(&c, 1);
        if (size != 1) {
            return "";
        }

        if (needNewLine) {
            if (c == '\n') {
                break;
            } else {
                printf("Expected newline in parseRequestHeader\n");
                return "";
            }
        }
        
        if (c == '\r') {
            needNewLine = true;
        } else {
            s += c;
        }
    }
    return s;
}

std::string
HTTPParser::urlDecode(const std::string& s)
{
    // Convert '+' to space
    std::string result = s;
    std::replace( result.begin(), result.end(), '+', ' ');
    
    // Convert % hex codes
    std::vector<std::string> parts = split(result, '%');
    result = parts[0];
    parts.erase(parts.begin());
    
    for (const auto& it : parts) {
        // First 2 chars are the hex code
        if (it.size() < 2) {
            printf("**** Error: invalid hex code\n");
            return "";
        }
        
        char buf[3] = "XX";
        buf[0] = it[0];
        buf[1] = it[1];
        uint8_t hex = strtol(buf, nullptr, 16);
        result += char(hex);
        result += it.substr(2);
    }
    
    return result;
}

std::vector<std::string>
HTTPParser::split(const std::string& str, char sep)
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

std::string
HTTPParser::trimWhitespace(const std::string& s)
{
    std::string returnString(s);
    returnString.erase(0, returnString.find_first_not_of(" \t"));
    returnString.erase(returnString.find_last_not_of(" \t") + 1);
    return returnString;
}

std::string
HTTPParser::removeQuotes(const std::string& s)
{            
    if (s[0] == '"') {
        std::string returnString = s.substr(1);
        returnString.pop_back();
        return returnString;
    }
    return s;
}

// Returns empty string if file is not a source file (cannot be displayed in web browser)
std::string
HTTPParser::suffixToMimeType(const std::string& filename)
{
    std::string suffix = std::filesystem::path(filename).extension();
    for_each(suffix.begin(), suffix.end(), [](char& c) {
        c = std::tolower(c);
    });

    if (suffix == ".htm" || suffix == ".html") {
        return "text/html";
    } else if (suffix == ".css") {
        return "text/css";
    } else if (suffix == ".js") {
        return "text/javascript";
    } else if (suffix == ".png") {
        return "image/png";
    } else if (suffix == ".gif") {
        return "image/gif";
    } else if (suffix == ".jpg" || suffix == ".jpeg") {
        return "image/jpeg";
    } else if (suffix == ".pdf") {
        return "application/pdf";
    } else if (suffix == ".txt" || suffix == ".c" || suffix == ".h" || suffix == ".cpp" || suffix == ".hpp") {
        return "text/plain";
    } else {
        return "";
    }
}

// Key/value pairs is separated by ':'
std::vector<std::string>
HTTPParser::parseKeyValue(const std::string& s)
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
std::vector<std::string>
HTTPParser::parseFormData(const std::string& value)
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

void
HTTPParser::parseQuery(const std::string& query)
{
    // parseQuery expects to see a leading '?'. Add it if it's not there
    std::regex paramRegex("([^&=]+)=([^&]*)");
    auto begin = std::sregex_iterator(query.begin(), query.end(), paramRegex);
    auto end = std::sregex_iterator();
    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        _args[match[1]] = match[2];
    }
}

bool
HTTPParser::parseRequestHeader(ReadCB cb)
{
    bool isFirstLine = true;
    
    while (true) {
        // Get a line
        std::string line = getLine(cb);
        
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
            _method = parsedLine[0];
            _path = parsedLine[1];
        } else {
            std::vector<std::string> keyValue = parseKeyValue(line);
            _headers[keyValue[0]] = keyValue[1];
        }
    }

    // Pick out args
    size_t firstArg = _path.find('?');
    if (firstArg == std::string::npos) {
        return true;
    }
    
    std::string argList = _path.substr(firstArg + 1);
    _path = _path.substr(0, firstArg);

    parseQuery(argList);
    
    return true;
}

#endif
