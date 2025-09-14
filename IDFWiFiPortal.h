/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "WiFiPortal.h"

class IDFWiFiPortal : public WiFiPortal
{
public:
    virtual void begin();

    virtual void resetSettings() override;
    virtual void setTitle(String title) override;
    virtual void setMenu(std::vector<const char*>& menu) override;
    virtual void setDarkMode(bool) override;
    virtual String getPrefString(const char* id) override;
    virtual void putPrefString(const char* id, const char* value) override;
    virtual void setCustomMenuHTML(const char* html) override;
    virtual void setHostname(const char*) override;
    virtual void setConfigHandler(std::function<void(WiFiPortal*)>) override;
    virtual void setShowInfoErase(bool enabled) override;
    virtual int32_t addHTTPHandler(const char* endpoint, HandleRequestCB handler) override;
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) override;
    virtual void process() override;
    virtual void startWebPortal() override;
    virtual String localIP() override;
    virtual const char* getSSID() override;
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const String& data = String()) override;
    virtual int readHTTPContent(uint8_t* buf, size_t bufSize) override;
    virtual size_t httpContentLength() override;
};

