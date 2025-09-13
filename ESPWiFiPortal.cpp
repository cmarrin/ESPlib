/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "ESPWiFiPortal.h"

#include <WiFiManager.h>
#include "Preferences.h"

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif

WiFiManager _wifiManager;
Preferences _prefs;

static WiFiPortal::HTTPMethod arduinoToPortalHTTPMethod(HTTPMethod method)
{
    switch(method) {
        case HTTP_GET: return WiFiPortal::HTTPMethod::Get;
        case HTTP_POST: return WiFiPortal::HTTPMethod::Post;
        default: return WiFiPortal::HTTPMethod::Any;
    }
}

void
ESPWiFiPortal::begin()
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
ESPWiFiPortal::setTitle(String title)
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

String
ESPWiFiPortal::getPrefString(const char* id)
{
    return _prefs.getString(id);
}

void
ESPWiFiPortal::putPrefString(const char* id, const char* value)
{
    _prefs.putString(id, value);
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
ESPWiFiPortal::setSaveParamsCallback(HandlerCB f)
{
    _wifiManager.setSaveParamsCallback([this, f]() { f(this); });
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
ESPWiFiPortal::addHTTPHandler(const char* endpoint, HandleRequestCB f)
{
    _wifiManager.server->on(endpoint, [this, f]() { f(this, arduinoToPortalHTTPMethod(_wifiManager.server->method()), _wifiManager.server->uri()); });
    return 0;
}

bool
ESPWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    return _wifiManager.autoConnect(apName, apPassword);
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

    if (!MDNS.begin(getParamValue("hostname")))  {             
        printf("***** Error starting mDNS\n");
    } else {
        printf("mDNS started, hostname=%s\n", getParamValue("hostname"));
    }
}

String
ESPWiFiPortal::localIP()
{
    return WiFi.localIP().toString();
}

const char*
ESPWiFiPortal::getSSID()
{
    return "";
}

void
ESPWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const String& data)
{
}

int
ESPWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    return -1;
}

size_t
ESPWiFiPortal::httpContentLength()
{
    return 0;
}
