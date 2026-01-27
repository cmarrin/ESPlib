/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "MacWiFiPortal.h"

using namespace mil;

void
MacWiFiPortal::begin(WebFileSystem* wfs)
{
    _server.start(wfs, 80);
}

void
MacWiFiPortal::resetSettings()
{
}

void
MacWiFiPortal::setTitle(const char* title)
{
}

void
MacWiFiPortal::setCustomMenuHTML(const char* html)
{
}

void
MacWiFiPortal::setHostname(const char*)
{
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
    delay(1);
}

void
MacWiFiPortal::startWebPortal()
{
}

std::string
MacWiFiPortal::localIP()
{
    return "localhost";
}

const char*
MacWiFiPortal::getSSID()
{
    return "";
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

std::string
MacWiFiPortal::getHTTPArg(const char* name)
{
    return _server.getHTTPArg(name);
}

bool
MacWiFiPortal::addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
{
//    // First we have to see if there is a saved value for this id. If so use it in place of the defaultValue
//    std::string value = _prefs.getString(id).c_str();
//    if (value.length() == 0) {
//        value = defaultValue;
//        printf("No '%s' saved. Setting it to default: '%s'\n", id, defaultValue);
//    } else {
//        printf("Setting '%s' to saved value: '%s'\n", id, value.c_str());
//    }
//
//    _prefs.putString(id, value.c_str());
//
//    _wifiManager.addParameter(new WiFiManagerParameter(id, label, value.c_str(), maxLength));
//    return true;
    return false;
}

bool
MacWiFiPortal::getParamValue(const char* id, std::string& value)
{
//    WiFiManagerParameter** params = _wifiManager.getParameters();
//    int count = _wifiManager.getParametersCount();
//    
//    for (int i = 0; i < count; ++i) {
//        if (strcmp(params[i]->getID(), id) == 0) {
//            return params[i]->getValue();
//        }
//    }
//    return nullptr;
    return false;
}

#endif
