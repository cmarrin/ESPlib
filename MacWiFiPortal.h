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
    virtual void setMenu(std::vector<const char*>& menu) override;
    virtual void setDarkMode(bool) override;
    virtual void setCustomMenuHTML(const char* html) override;
    virtual void setHostname(const char*) override;
    virtual void setConfigHandler(std::function<void(WiFiPortal*)>) override;
    virtual void setShowInfoErase(bool enabled) override;
    virtual int32_t addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB = nullptr) override;
    virtual void serveStatic(const char *uri, const char *path) override;
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) override;
    virtual void process() override;
    virtual void startWebPortal() override;
    virtual std::string localIP() override;
    virtual const char* getSSID() override;
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "") override;
    virtual void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip) override;
    virtual void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach) override;
    virtual int readHTTPContent(uint8_t* buf, size_t bufSize) override;
    virtual HTTPUploadStatus httpUploadStatus() const override;
    virtual std::string httpUploadFilename() const override;
    virtual std::string httpUploadName() const override;
    virtual std::string httpUploadType() const override;
    virtual size_t httpUploadTotalSize() const override;
    virtual size_t httpUploadCurrentSize() const override;
    virtual const uint8_t* httpUploadBuffer() const override;
    virtual std::string getHTTPArg(const char* name) override;
    virtual bool addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength) override;
    virtual const char* getParamValue(const char* id) override;
    
private:
    WebServer _server;
};

}
