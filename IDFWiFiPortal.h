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
    void begin();

    void resetSettings();
    void setTitle(String title);
    void setMenu(std::vector<const char*>& menu);
    void setDarkMode(bool);
    String getPrefString(const char* id);
    void putPrefString(const char* id, const char* value);
    void setCustomMenuHTML(const char* html);
    void setHostname(const char*);
    void setSaveParamsCallback(std::function<void(WiFiPortal*)>);
    void setConfigHandler(std::function<void(WiFiPortal*)>);
    void setShowInfoErase(bool enabled);
    void addHTTPHandler(const char* page, std::function<void(WiFiPortal*)> handler);
    void addHTTPHandler(HTTPRequest*);
    bool autoConnect(char const *apName, char const *apPassword = NULL);
    void process();
    void startWebPortal();
    String localIP();
    const char* getSSID();
    String getHTTPArg(const char*);
    void sendHTTPResponse(int code, const char* mimetype = nullptr, const String& data = String());
};

