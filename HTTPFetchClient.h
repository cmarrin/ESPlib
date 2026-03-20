/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2026, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

// HTTPClient
//
// Handle simple REST requests. This uses HTTPClient on Arduino, curl on
// Mac and esp_http_client on espidf

namespace mil {

class HTTPClient
{
public:
    HTTPClient(std::function<void(const char* buf, uint32_t size)> cb) : _handler(cb) { }
    
    bool fetch(const char* url);
    
    std::function<void(const char* buf, uint32_t size)> _handler;
};

}
