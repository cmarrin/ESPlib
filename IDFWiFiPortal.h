/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "WiFiPortal.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>              
#include <esp_http_server.h>        

#include <nvs_flash.h>

#include <map>

namespace mil {

class IDFWiFiPortal : public WiFiPortal
{
public:
    virtual void begin(WebFileSystem*) override;

    virtual void resetSettings() override;
    virtual void setTitle(const char* title) override;
    virtual void setMenu(std::vector<const char*>& menu) override;
    virtual void setDarkMode(bool) override;
    virtual void setCustomMenuHTML(const char* html) override;
    virtual void setHostname(const char*) override;
    virtual void setConfigHandler(HandlerCB) override;
    virtual void setShowInfoErase(bool enabled) override;
    virtual int32_t addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB = nullptr) override;
    virtual void serveStatic(const char *uri, const char *path) override;
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) override;
    virtual void process() override;
    virtual void startWebPortal() override;
    virtual std::string localIP() override { return _currentIP; }
    virtual const char* getSSID() override { return _ssid.c_str(); }
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
    virtual bool getParamValue(const char* id, std::string& value) override;
    
  private:
    static constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
    static constexpr EventBits_t WIFI_FAIL_BIT = BIT1;

    bool isConnected() const { return _isConnected; }
    void stopWiFi();
    void stopWebServer();
    void startProvisioning();
    
    void scanNetworks();
    
    static void eventHandler(void* arg, esp_event_base_t, int32_t event_id, void* event_data);
    static esp_err_t provisioningGetHandler(httpd_req_t*);
    static esp_err_t connectPostHandler(httpd_req_t*);
    static esp_err_t resetGetHandler(httpd_req_t*);
    static esp_err_t getKnownNetworksHandler(httpd_req_t*);
    static esp_err_t faviconGetHandler(httpd_req_t*);

    // Params are stored in nvs memory, but also need to be presented to the user in the web page.
    // So we store a list here with the label to display and the max length. The key for each entry
    // is the param id.
    void setNVSParam(const char* id, const std::string& value);
    bool getNVSParam(const char* id, std::string& value);
    void eraseNVSParam(const char* id);
    
    struct MapValue { std::string label; uint32_t maxLength; };
    
    std::map<std::string, MapValue> _paramMap;
    
    struct KnownNetwork
    {
        bool operator==(const KnownNetwork& other) const { return ssid == other.ssid; }
        bool operator<(const KnownNetwork& other) const { return ssid < other.ssid; }
        std::string ssid; 
        int8_t rssi; 
        bool open;
    };
    std::vector<KnownNetwork> _knownNetworks;
    
    bool _isConnected = false;
    EventGroupHandle_t _eventGroup;
    httpd_handle_t _server = nullptr;
    uint8_t _retryNum = 0;
    std::string _currentIP;
    std::string _ssid;
    std::string _pass;
    
    WebFileSystem* _wfs = nullptr;
    
    static constexpr uint8_t MAX_RETRY = 5;
    
    // During an active request this has a valid pointer. Otherwise it is null
    httpd_req_t* _activeRequest = nullptr;
};

}
