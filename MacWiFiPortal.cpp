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
MacWiFiPortal::setMenu(std::vector<const char*>& menu)
{
}

void
MacWiFiPortal::setDarkMode(bool)
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

void
MacWiFiPortal::setShowInfoErase(bool enabled)
{
}

int32_t
MacWiFiPortal::addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB)
{
   _server.addHTTPHandler(endpoint, [this, requestCB]() { requestCB(this); }, [this, uploadCB]() { uploadCB(this); });
    return 0;
}

void
MacWiFiPortal::serveStatic(const char *uri, const char *path)
{
    _server.serveStatic(uri, path);
}

bool
MacWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    return true;
}

void
MacWiFiPortal::process()
{
}

void
MacWiFiPortal::startWebPortal()
{
}

std::string
MacWiFiPortal::localIP()
{
    return "";
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
MacWiFiPortal::streamHTTPResponse(File& file, const char* mimetype, bool attach)
{
//    // For now assume this is a file download. So set Content-Disposition
//    std::string disp = attach ? "attachment" : "inline";
//    disp += "; filename=\"";
//    disp += file.name();
//    disp += "\"";
//    _wifiManager.server->sendHeader("Content-Disposition", disp.c_str(), true);
//    _wifiManager.server->streamFile(file, mimetype);
}

int
MacWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    return -1;
}

WiFiPortal::HTTPUploadStatus
MacWiFiPortal::httpUploadStatus() const
{
//    switch(_wifiManager.server->upload().status) {
//        default: return HTTPUploadStatus::None;
//        case UPLOAD_FILE_START: return HTTPUploadStatus::Start;
//        case UPLOAD_FILE_WRITE: return HTTPUploadStatus::Write;
//        case UPLOAD_FILE_END: return HTTPUploadStatus::End;
//        case UPLOAD_FILE_ABORTED: return HTTPUploadStatus::Aborted;
//    }

    return HTTPUploadStatus::None;
}

std::string
MacWiFiPortal::httpUploadFilename() const
{
//    return _wifiManager.server->upload().filename.c_str();
    return "";
}

std::string
MacWiFiPortal::httpUploadName() const
{
//    return _wifiManager.server->upload().name.c_str();
    return "";
}

std::string
MacWiFiPortal::httpUploadType() const
{
//    return _wifiManager.server->upload().type.c_str();
    return "";
}

size_t
MacWiFiPortal::httpUploadTotalSize() const
{
//    return _wifiManager.server->upload().totalSize;
    return 0;
}

size_t
MacWiFiPortal::httpUploadCurrentSize() const
{
//    return _wifiManager.server->upload().currentSize;
    return 0;
}

const uint8_t*
MacWiFiPortal::httpUploadBuffer() const
{
//    return _wifiManager.server->upload().buf;
    return nullptr;
}

std::string
MacWiFiPortal::getHTTPArg(const char* name)
{
//    return _wifiManager.server->arg(name).c_str();
    return "";
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

const char*
MacWiFiPortal::getParamValue(const char* id)
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
    return "";
}

#endif
