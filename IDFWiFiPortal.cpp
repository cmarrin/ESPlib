/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "IDFWiFiPortal.h"

#include "WebFileSystem.h"

using namespace mil;

static const char* TAG = "WiFiPortal";
static const char* PROV_AP_SSID = "ESP32-Provisioning";
static const char* PROV_AP_PASS = "password123";
static constexpr uint32_t PROV_AP_MAX_CONN = 1;

void
IDFWiFiPortal::begin(WebFileSystem* wfs)
{
    _wfs = wfs;
    
    // Initialize NVS partition to hold params
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());

        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    _eventGroup = xEventGroupCreate();
}

void
IDFWiFiPortal::resetSettings()
{
    eraseNVSParam("wifi_ssid");
    eraseNVSParam("wifi_pass");
}

void
IDFWiFiPortal::setTitle(const char* title)
{
}

void
IDFWiFiPortal::setMenu(std::vector<const char*>& menu)
{
}

void
IDFWiFiPortal::setDarkMode(bool b)
{
}

void
IDFWiFiPortal::setCustomMenuHTML(const char* html)
{
}

void
IDFWiFiPortal::setHostname(const char* name)
{
    printf("*** setHostname not implemented\n");
}

void
IDFWiFiPortal::setConfigHandler(HandlerCB f)
{
    printf("*** setConfigHandler not implemented\n");
}

void
IDFWiFiPortal::setShowInfoErase(bool enabled)
{
}

int32_t
IDFWiFiPortal::addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB)
{
    printf("*** addHTTPHandler not implemented\n");
    return 0;
}

void
IDFWiFiPortal::serveStatic(const char *uri, const char *path)
{
    printf("*** serveStatic not implemented\n");
}

bool
IDFWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    // Initialize
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    scanNetworks();
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &eventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &eventHandler,
                                                        this,
                                                        nullptr));

    getParamValue("wifi_ssid", _ssid);
    getParamValue("wifi_pass", _pass);
    
    if (!_ssid.empty()) {
        // Connect to WiFi
        ESP_LOGI(TAG, "Found credentials, connecting to '%s'", _ssid.c_str());

        stopWiFi();
        
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config_t wifi_config = {};
        strncpy((char*)wifi_config.sta.ssid, _ssid.c_str(), sizeof(wifi_config.sta.ssid));
        if (_ssid.length() < sizeof(wifi_config.sta.ssid)) {
            wifi_config.sta.ssid[_ssid.length()] = '\0';
        }
        strncpy((char*)wifi_config.sta.password, _pass.c_str(), sizeof(wifi_config.sta.password));
        if (_pass.length() < sizeof(wifi_config.sta.password)) {
            wifi_config.sta.password[_pass.length()] = '\0';
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        EventBits_t bits = xEventGroupWaitBits(_eventGroup,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(30000));

        if (bits & WIFI_CONNECTED_BIT) {
            startWebServer();
            return true;
        } 
        ESP_LOGW(TAG, "Failed to connect with stored credentials");
    } else {
        ESP_LOGI(TAG, "Credentials not found");
    }
    
    ESP_LOGI(TAG, "Starting captive portal");
    startProvisioning();
    return true;
}

void
IDFWiFiPortal::process()
{
}

void
IDFWiFiPortal::startWebPortal()
{
    printf("*** startWebPortal not implemented\n");
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
    printf("*** sendHTTPResponse (1) not implemented\n");
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
    if (!_activeRequest) {
        ESP_LOGE(TAG, "Can't send HTTP response. No active request.");
        return;
    }

    ESP_ERROR_CHECK(httpd_resp_set_type(_activeRequest, mimetype));
    ESP_ERROR_CHECK(httpd_resp_send(_activeRequest, data, length));
}    

void
IDFWiFiPortal::streamHTTPResponse(fs::File& file, const char* mimetype, bool attach)
{
    printf("*** streamHTTPResponse not implemented\n");
}

int
IDFWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    printf("*** readHTTPContent not implemented\n");
    return -1;
}

WiFiPortal::HTTPUploadStatus
IDFWiFiPortal::httpUploadStatus() const
{
    return HTTPUploadStatus::None;
}

std::string
IDFWiFiPortal::httpUploadFilename() const
{
    return "not implemented";
}

std::string
IDFWiFiPortal::httpUploadName() const
{
    return "not implemented";
}

std::string
IDFWiFiPortal::httpUploadType() const
{
    return "not implemented";
}

size_t
IDFWiFiPortal::httpUploadTotalSize() const
{
    return 0;
}

size_t
IDFWiFiPortal::httpUploadCurrentSize() const
{
    return 0;
}

const uint8_t*
IDFWiFiPortal::httpUploadBuffer() const
{
    return nullptr;
}

std::string
IDFWiFiPortal::getHTTPArg(const char* name)
{
    return "not implemented";
}

bool
IDFWiFiPortal::addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
{
    // See if the value is already in the map
    auto entry = _paramMap.find(id);
    if (entry == _paramMap.end()) {
        // New entry
        setNVSParam(id, defaultValue);
        _paramMap.insert({ id, { label, maxLength } });
    } else {
        // Existing entry
        setNVSParam(id, defaultValue);
    }
    return false;
}

bool
IDFWiFiPortal::getParamValue(const char* id, std::string& value)
{
    return getNVSParam(id, value);
}

void
IDFWiFiPortal::setNVSParam(const char* id, const std::string& value)
{
    nvs_handle_t paramHandle;

    ESP_ERROR_CHECK(nvs_open("esplib", NVS_READWRITE, &paramHandle));    
    ESP_ERROR_CHECK(nvs_set_str(paramHandle, id, value.c_str()));
    ESP_ERROR_CHECK(nvs_commit(paramHandle));
    nvs_close(paramHandle);
}

bool
IDFWiFiPortal::getNVSParam(const char* id, std::string& value)
{
    nvs_handle_t paramHandle;

    ESP_ERROR_CHECK(nvs_open("esplib", NVS_READONLY, &paramHandle));

    size_t requiredSize = 0;
    esp_err_t err = nvs_get_str(paramHandle, id, NULL, &requiredSize);
    if (err == ESP_OK) {
        std::vector<char> ssidBuf(requiredSize);
        err = nvs_get_str(paramHandle, id, ssidBuf.data(), &requiredSize);
        if (err == ESP_OK) {
            value = ssidBuf.data();
        } else {
            ESP_LOGE(TAG, "Error (%s) getting string param", esp_err_to_name(err));
        }
    } else {
        value = "";
    }

    nvs_close(paramHandle);
    return err == ESP_OK;
}

void
IDFWiFiPortal::eraseNVSParam(const char* id)
{
    nvs_handle_t paramHandle;

    ESP_ERROR_CHECK(nvs_open("esplib", NVS_READWRITE, &paramHandle));
    esp_err_t err = nvs_erase_key(paramHandle, id);
    if (err == ESP_OK) {
        ESP_ERROR_CHECK(nvs_commit(paramHandle));
    }
    nvs_close(paramHandle);
}

void
IDFWiFiPortal::stopWiFi()
{
    stopWebServer();
    esp_wifi_stop();
    esp_wifi_deinit();
    
    esp_netif_t* netif_sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif_sta) {
        esp_netif_destroy(netif_sta);
    }
    esp_netif_t* netif_ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif_ap) {
        esp_netif_destroy(netif_ap);
    }
}

void
IDFWiFiPortal::startWebServer()
{
    if (_server) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;

    if (httpd_start(&_server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {.uri = "/", .method = HTTP_GET, .handler = provisioningGetHandler, .user_ctx = this };
        httpd_register_uri_handler(_server, &root_uri);
        
        httpd_uri_t connect_uri = {.uri = "/connect", .method = HTTP_POST, .handler = connectPostHandler, .user_ctx = this };
        httpd_register_uri_handler(_server, &connect_uri);
        
        httpd_uri_t reset_uri = {.uri = "/reset", .method = HTTP_GET, .handler = resetGetHandler, .user_ctx = this };
        httpd_register_uri_handler(_server, &reset_uri);
        
        httpd_uri_t favicon_uri = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = faviconGetHandler, .user_ctx = this };
        httpd_register_uri_handler(_server, &favicon_uri);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}

void
IDFWiFiPortal::stopWebServer()
{
    if (_server) {
        httpd_stop(_server);
        _server = nullptr;
    }
}

void
IDFWiFiPortal::startProvisioning()
{
    stopWiFi();

    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, PROV_AP_SSID, sizeof(wifi_config.ap.ssid));
    strncpy((char*)wifi_config.ap.password, PROV_AP_PASS, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(PROV_AP_SSID);
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = PROV_AP_MAX_CONN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    ESP_LOGI(TAG, "AP started. SSID: '%s', Connect to http://" IPSTR, PROV_AP_SSID, IP2STR(&ip_info.ip));

    startWebServer();
}

void
IDFWiFiPortal::scanNetworks()
{
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_set_mode(WIFI_MODE_STA);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &eventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &eventHandler,
                                                        this,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_disconnect());

    esp_wifi_scan_start(nullptr, true);

    uint16_t apCount = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
    ESP_LOGI(TAG, "Total APs scanned = %u", apCount);
    if (apCount) {
        std::vector<wifi_ap_record_t> apInfo(apCount);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, apInfo.data()));
        
        // Copy the ssids to list
        _knownNetworks.clear();
        for (auto& it : apInfo) {
            _knownNetworks.push_back(std::string(reinterpret_cast<const char*>(it.ssid)));
        }

        // Get rid of duplicates
        std::sort(_knownNetworks.begin(), _knownNetworks.end());
        _knownNetworks.erase(std::unique(_knownNetworks.begin(), _knownNetworks.end()), _knownNetworks.end());
    
        for (auto& it : _knownNetworks) {
            ESP_LOGI(TAG, "SSID \t\t%s", it.c_str());
        }
    }
}

void
IDFWiFiPortal::eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(arg);

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->_retryNum < MAX_RETRY) {
            esp_wifi_connect();
            self->_retryNum++;
        } else {
            self->_isConnected = false;
            xEventGroupSetBits(self->_eventGroup, WIFI_FAIL_BIT);
        }
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = reinterpret_cast<ip_event_got_ip_t*>(eventData);
        char ipStr[16];
        snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&event->ip_info.ip));
        self->_currentIP = ipStr;
        self->_retryNum = 0;
        self->_isConnected = true;
        xEventGroupSetBits(self->_eventGroup, WIFI_CONNECTED_BIT);
    }
}

esp_err_t
IDFWiFiPortal::provisioningGetHandler(httpd_req_t *req)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(req->user_ctx);
    self->_activeRequest = req;
    
    // Send the portal page
    if (self->_wfs) {
        self->_wfs->sendPortalPage(self);
    }
    return ESP_OK;
}

esp_err_t
IDFWiFiPortal::connectPostHandler(httpd_req_t *req)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(req->user_ctx);
    
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char ssidBuf[33] = {0};
    char passBuf[65] = {0};
    
    if (httpd_query_key_value(buf, "ssid", ssidBuf, sizeof(ssidBuf)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'ssid' parameter");
        return ESP_FAIL;
    }
    httpd_query_key_value(buf, "password", passBuf, sizeof(passBuf));
    
    // Decode the strings
    std::string ssid = WebFileSystem::urlDecode(ssidBuf);
    std::string pass = WebFileSystem::urlDecode(passBuf);

    std::string response = "<h1>Connecting to '";
    response += ssid;
    response += "'...</h1><p>If successful, the device will connect to the network. If failed, it will remain in provisioning mode.</p>";
    httpd_resp_sendstr(req, response.c_str());

    self->setNVSParam("wifi_ssid", ssid);
    self->setNVSParam("wifi_pass", pass);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

esp_err_t
IDFWiFiPortal::resetGetHandler(httpd_req_t* req)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(req->user_ctx);
    
    const char* resp_str = "<h1>Credentials Cleared</h1><p>The device will restart and enter provisioning mode.</p>";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    self->resetSettings();
    esp_restart();
    
    return ESP_OK;
}

esp_err_t
IDFWiFiPortal::faviconGetHandler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

#endif
