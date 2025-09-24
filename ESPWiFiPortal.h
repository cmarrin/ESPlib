/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "WiFiPortal.h"

#include <WiFiManager.h>
#include "Preferences.h"

class ESPWiFiPortal : public WiFiPortal
{
public:
    virtual void begin() override;

    virtual void resetSettings() override;
    virtual void setTitle(const char* title) override;
    virtual void setMenu(std::vector<const char*>& menu) override;
    virtual void setDarkMode(bool) override;
    virtual void setCustomMenuHTML(const char* html) override;
    virtual void setHostname(const char*) override;
    virtual void setConfigHandler(HandlerCB) override;
    virtual void setShowInfoErase(bool enabled) override;
    virtual int32_t addHTTPHandler(const char* endpoint, HandleRequestCB) override;
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) override;
    virtual void process() override;
    virtual void startWebPortal() override;
    virtual std::string localIP() override;
    virtual const char* getSSID() override;
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const std::string& data = "") override;
    virtual int readHTTPContent(uint8_t* buf, size_t bufSize) override;
    virtual size_t httpContentLength() override;
    virtual std::string getHTTPArg(const std::string& uri, const char* name) override;
    virtual bool addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength) override;
    virtual const char* getParamValue(const char* id) override;

private:
    WiFiManager _wifiManager;
    Preferences _prefs;


};

