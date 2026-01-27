/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "WiFiPortal.h"

#include "MacWebServer.h"

namespace mil {

class MacWiFiPortal : public WiFiPortal
{
public:
    MacWiFiPortal() { }
    virtual ~MacWiFiPortal() { }
    
    virtual void begin(WebFileSystem*) override;

    virtual void resetSettings() override;
    virtual void setTitle(const char* title) override;
    virtual void setCustomMenuHTML(const char* html) override;
    virtual void setHostname(const char*) override;
    virtual void setConfigHandler(std::function<void(WiFiPortal*)>) override;
    virtual int32_t addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB) override;
    virtual void addStaticHTTPHandler(const char *uri, const char *path) override;
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) override;
    virtual void process() override;
    virtual void startWebPortal() override;
    virtual std::string localIP() override;
    virtual const char* getSSID() override;
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "") override;
    virtual void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip) override;
    virtual void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach) override;
    virtual HTTPUploadStatus httpUploadStatus() const override;
    virtual std::string httpUploadFilename() const override;
    virtual size_t httpUploadTotalSize() const override;
    virtual size_t httpUploadCurrentSize() const override;
    virtual const uint8_t* httpUploadBuffer() const override;
    virtual std::string getHTTPArg(const char* name) override;
    virtual bool addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength) override;
    virtual bool getParamValue(const char* id, std::string& value) override;
    
private:
    std::string _hostname;
    std::string _title;
    std::string _customHTML;
    std::chrono::steady_clock::time_point _startTime;

    WebServer _server;
};

}
