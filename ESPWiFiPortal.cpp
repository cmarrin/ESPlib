/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "ESPWiFiPortal.h"

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif

#include "LittleFS.h"

using namespace mil;

void
ESPWiFiPortal::begin(WebFileSystem*)
{
    _prefs.begin("ESPLib");
    _wifiManager.setDebugOutput(true);
}

void
ESPWiFiPortal::resetSettings()
{
    _wifiManager.resetSettings();			
}

void
ESPWiFiPortal::setTitle(const char* title)
{
    _wifiManager.setTitle(title);
}

void
ESPWiFiPortal::setMenu(std::vector<const char*>& menu)
{
    _wifiManager.setMenu(menu);
}

void
ESPWiFiPortal::setDarkMode(bool b)
{
    _wifiManager.setDarkMode(b);
}

void
ESPWiFiPortal::setCustomMenuHTML(const char* html)
{
    _wifiManager.setCustomMenuHTML(html);
}

void
ESPWiFiPortal::setHostname(const char* name)
{
    _wifiManager.setHostname(name);
}

void
ESPWiFiPortal::setConfigHandler(HandlerCB f)
{
    _wifiManager.setAPCallback([this, f](WiFiManager*) { f(this); });
}

void
ESPWiFiPortal::setShowInfoErase(bool enabled)
{
    _wifiManager.setShowInfoErase(enabled);
}

int32_t
ESPWiFiPortal::addHTTPHandler(const char* endpoint, HTTPMethod, HandlerCB requestCB, HandlerCB uploadCB)
{
    _wifiManager.server->on(endpoint, HTTP_ANY, [this, requestCB]() { requestCB(this); }, [this, uploadCB]() { uploadCB(this); });
    return 0;
}

void
ESPWiFiPortal::serveStatic(const char *uri, const char *path)
{
    _wifiManager.server->serveStatic(uri, LittleFS, path);
}

bool
ESPWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    bool result = _wifiManager.autoConnect(apName, apPassword);

    _wifiManager.setSaveParamsCallback([this]()
    {
        WiFiManagerParameter** params = _wifiManager.getParameters();
        int count = _wifiManager.getParametersCount();
        
        for (int i = 0; i < count; ++i) {
            _prefs.putString(params[i]->getID(), params[i]->getValue());
        }
        
        // Restart so the new settings take effect
        ESP.restart();
        delay(1000);
    });

    return result;
}

void
ESPWiFiPortal::process()
{
#if defined(ESP8266)
    MDNS.update();
#endif
    _wifiManager.process();
}

void
ESPWiFiPortal::startWebPortal()
{
    _wifiManager.startWebPortal();

    std::string hostname;
    if (!getParamValue("hostname", hostname)) {
        printf("***** can't start mDNS. No hostname\n");
    } else if (!MDNS.begin(hostname.c_str()))  {             
        printf("***** Error starting mDNS\n");
    } else {
        printf("mDNS started, hostname=%s\n", hostname.c_str());
    }
}

std::string
ESPWiFiPortal::localIP()
{
    return WiFi.localIP().toString().c_str();
}

const char*
ESPWiFiPortal::getSSID()
{
    return "";
}

void
ESPWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
    _wifiManager.server->send(code, mimetype, data);
}

void
ESPWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
    if (gzip) {
        _wifiManager.server->sendHeader("Content-Encoding", "gzip", true);
    }
    
    _wifiManager.server->setContentLength(length);
    _wifiManager.server->send(code, mimetype);
    _wifiManager.server->sendContent(data, length);
}    

void
ESPWiFiPortal::streamHTTPResponse(fs::File& file, const char* mimetype, bool attach)
{
    // For now assume this is a file download. So set Content-Disposition
    std::string disp = attach ? "attachment" : "inline";
    disp += "; filename=\"";
    disp += file.name();
    disp += "\"";
    _wifiManager.server->sendHeader("Content-Disposition", disp.c_str(), true);
    _wifiManager.server->streamFile(file, mimetype);
}

int
ESPWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    return -1;
}

WiFiPortal::HTTPUploadStatus
ESPWiFiPortal::httpUploadStatus() const
{
    switch(_wifiManager.server->upload().status) {
        default: return HTTPUploadStatus::None;
        case UPLOAD_FILE_START: return HTTPUploadStatus::Start;
        case UPLOAD_FILE_WRITE: return HTTPUploadStatus::Write;
        case UPLOAD_FILE_END: return HTTPUploadStatus::End;
        case UPLOAD_FILE_ABORTED: return HTTPUploadStatus::Aborted;
    }
}

std::string
ESPWiFiPortal::httpUploadFilename() const
{
    return _wifiManager.server->upload().filename.c_str();
}

std::string
ESPWiFiPortal::httpUploadName() const
{
    return _wifiManager.server->upload().name.c_str();
}

std::string
ESPWiFiPortal::httpUploadType() const
{
    return _wifiManager.server->upload().type.c_str();
}

size_t
ESPWiFiPortal::httpUploadTotalSize() const
{
    return _wifiManager.server->upload().totalSize;
}

size_t
ESPWiFiPortal::httpUploadCurrentSize() const
{
    return _wifiManager.server->upload().currentSize;
}

const uint8_t*
ESPWiFiPortal::httpUploadBuffer() const
{
    return _wifiManager.server->upload().buf;
}

std::string
ESPWiFiPortal::getHTTPArg(const char* name)
{
    return _wifiManager.server->arg(name).c_str();
}

bool
ESPWiFiPortal::addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
{
    // First we have to see if there is a saved value for this id. If so use it in place of the defaultValue
    std::string value = _prefs.getString(id).c_str();
    if (value.length() == 0) {
        value = defaultValue;
        printf("No '%s' saved. Setting it to default: '%s'\n", id, defaultValue);
    } else {
        printf("Setting '%s' to saved value: '%s'\n", id, value.c_str());
    }

    _prefs.putString(id, value.c_str());

    _wifiManager.addParameter(new WiFiManagerParameter(id, label, value.c_str(), maxLength));
    return true;
}

bool
ESPWiFiPortal::getParamValue(const char* id, std::string& value)
{
    WiFiManagerParameter** params = _wifiManager.getParameters();
    int count = _wifiManager.getParametersCount();
    
    for (int i = 0; i < count; ++i) {
        if (strcmp(params[i]->getID(), id) == 0) {
            value = params[i]->getValue();
            return true;
        }
    }
    return false;
}
