/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "MacWiFiPortal.h"

#include "WebFileSystem.h"

using namespace mil;

void
MacWiFiPortal::begin(WebFileSystem* wfs)
{
    _startTime = std::chrono::steady_clock::now();

    _server.start(wfs, 80);
    
    // On Mac we'll store NVS params in a special /.nvs dir in the file system
    // Each file is the key name and the contents are the value
    if (!WebFileSystem::exists("/.nvs")) {
        WebFileSystem::mkdir("/.nvs");
    }
    
    // Fill in _knownNetworks with some dummy data
    _knownNetworks.emplace_back("My Network", -50, false);
    _knownNetworks.emplace_back("I Am Open", -90, true);
    _knownNetworks.emplace_back("fishhook", -70, false);
    _knownNetworks.emplace_back("Suliban", -100, true);
}

void
MacWiFiPortal::setConfigHandler(HandlerCB)
{
}

int32_t
MacWiFiPortal::addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB)
{
   _server.addHTTPHandler(endpoint, method, [this, requestCB]() { requestCB(this); });
    return 0;
}

void
MacWiFiPortal::addStaticHTTPHandler(const char *uri, const char *path)
{
    _server.addStaticHTTPHandler(uri, path);
}

bool
MacWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    return true;
}

void
MacWiFiPortal::process()
{
    _server.process();
    System::delay(1);
}

std::string
MacWiFiPortal::getIP()
{
    return "localhost";
}

const char*
MacWiFiPortal::getSSID()
{
    return "My Network";
}

void
MacWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
    _server.sendHTTPResponse(code, mimetype, data);
}

void
MacWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
    _server.sendHTTPResponse(code, mimetype, data, length, gzip);
}    

void
MacWiFiPortal::streamHTTPResponse(fs::File& file, const char* mimetype, bool attach)
{
    _server.streamHTTPResponse(file, mimetype, attach);
}

WiFiPortal::HTTPUploadStatus
MacWiFiPortal::httpUploadStatus() const
{
    return _server.httpUploadStatus();
}

std::string
MacWiFiPortal::httpUploadFilename() const
{
    return _server.httpUploadFilename();
}

size_t
MacWiFiPortal::httpUploadTotalSize() const
{
    return _server.httpUploadTotalSize();
}

size_t
MacWiFiPortal::httpUploadCurrentSize() const
{
    return _server.httpUploadCurrentSize();
}

const uint8_t*
MacWiFiPortal::httpUploadBuffer() const
{
    return _server.httpUploadBuffer();
}

int
MacWiFiPortal::receiveHTTPResponse(char* buf, size_t size)
{
    return _server.receiveHTTPResponse(buf, size);
}

std::string
MacWiFiPortal::getCPUModel() const
{
    return "Mac";
}

uint32_t
MacWiFiPortal::getCPUUptime() const
{
    return (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - _startTime)).count();
}

void
MacWiFiPortal::setNVSParam(const char* id, const std::string& value)
{
    std::string keyName = std::string("/.nvs/") + id;
    fs::File f = WebFileSystem::open(keyName.c_str(), "w");
    f.write(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
    f.close();
}

bool
MacWiFiPortal::getNVSParam(const char* id, std::string& value) const
{
    std::string keyName = std::string("/.nvs/") + id;
    if (!WebFileSystem::exists(keyName.c_str())) {
        return false;
    }
    
    fs::File f = WebFileSystem::open(keyName.c_str(), "r");
    size_t size = f.size();
    uint8_t* buf = new uint8_t[size];
    f.read(buf, size);
    f.close();
    value = std::string(reinterpret_cast<char*>(buf), size);
    return true;
}

void
MacWiFiPortal::eraseNVSParam(const char* id)
{
    std::string keyName = std::string("/.nvs/") + id;
    WebFileSystem::remove(keyName.c_str());
}


#endif
