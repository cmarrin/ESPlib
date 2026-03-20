/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "WiFiPortal.h"

#include <WebServer.h>
#include <Preferences.h>

namespace mil {

class ESPWiFiPortal : public WiFiPortal
{
public:
    virtual void begin(WebFileSystem*) override;

    virtual void resetSettings() override;
    virtual void setConfigHandler(HandlerCB) override;
    virtual int32_t addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB) override;
    virtual void addStaticHTTPHandler(const char *uri, const char *path) override;
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) override;
    virtual void process() override;
    virtual void startWebPortal() override;
    virtual std::string getIP() override;
    virtual const char* getSSID() override;
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "") override;
    virtual void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip) override;
    virtual void streamHTTPResponse(File& file, const char* mimetype, bool attach) override;
    virtual HTTPUploadStatus httpUploadStatus() const override;
    virtual std::string httpUploadFilename() const override;
    virtual size_t httpUploadTotalSize() const override;
    virtual size_t httpUploadCurrentSize() const override;
    virtual const uint8_t* httpUploadBuffer() const override;
    virtual std::string getHTTPArg(const char* name) override;
    virtual bool hasHTTPArg(const char* name) override;
    virtual std::string getCPUModel() const override;
    virtual uint32_t getCPUFrequency() const override;
    virtual float getCPUTemperature() const override;
    virtual uint32_t getCPUUptime() const override;

    virtual void setNVSParam(const char* id, const std::string& value) override;
    virtual bool getNVSParam(const char* id, std::string& value) const override;
    virtual void eraseNVSParam(const char* id) override;

private:
    void scanNetworks();
    void startProvisioning();
    void startWebServer(bool provision);
    void connectHandler(WiFiPortal*);
    void restartGetHandler(WiFiPortal*);
    void resetGetHandler(WiFiPortal*);
    void getWifiSetupHandler(WiFiPortal*);
    
    void redirectRoot();
    
    Preferences _prefs;
    std::unique_ptr<WebServer> _server;
    WebFileSystem& _wfs = nullptr;
    HandlerCB _configHandler;

    struct KnownNetwork
    {
        bool operator==(const KnownNetwork& other) const { return ssid == other.ssid; }
        bool operator<(const KnownNetwork& other) const { return ssid < other.ssid; }
        std::string ssid; 
        int8_t rssi; 
        bool open;
    };
    std::vector<KnownNetwork> _knownNetworks;
};

}
