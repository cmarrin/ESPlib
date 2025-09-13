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
    _wifiManager.setSaveParamsCallback([this]()
    {
        // WiFiManager has had a param saved through the web page.
        // Grab all values from the WiFiManagerParameters list and
        // put them in _params
        WiFiManagerParameter** params = _wifiManager.getParameters();
        int paramCount = _wifiManager.getParametersCount();

        for (int i = 0; i < paramCount; ++i) {
            const char* id = params[i]->getID();
            const char* value = params[i]->getValue();
            _params[id].value = value;
            putPrefString(id, value);
        }
        
        // Restart so the new settings take effect
        ESP.restart();
        delay(1000);
    });
    
    // We need to introduce the params to WiFiManager. That requires us to
    // make a list of WiFiManagerParameter entries to match the Param list.
    //
    // FIXME: WiFiManager can't delete old params and it keeps a pointer
    // to the params in this list. So once we introduce the params to 
    // WiFiManager we can't add any more params. We need to add checks
    // for this.
    for (const auto& it : _params) {
        WiFiManagerParameter* param = new WiFiManagerParameter(it.first.c_str(), it.second.label.c_str(), it.second.value.c_str(), it.second.length);
        _wifiManager.addParameter(param);
    }
    
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
