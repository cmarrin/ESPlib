/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "IDFWiFiPortal.h"

#include "HTTPParser.h"
#include "WebFileSystem.h"

#include "dns_server.h"
#include "mdns.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "soc/rtc.h"
#include "driver/temperature_sensor.h"

using namespace mil;

static const char *TAG = "WiFiPortal";
static const char* PROV_AP_SSID = "ESP32-Provisioning";
static const char* PROV_AP_PASS = "password123";
static constexpr uint32_t PROV_AP_MAX_CONN = 4;

// HTTP Error (404) Handler when connected - show a 404 page
static esp_err_t connectedHTTP404ErrorHandler(httpd_req_t *req, httpd_err_code_t err)
{
    ESP_ERROR_CHECK(httpd_resp_set_status(req, "404 Not Found"));
    httpd_resp_send(req, "<h1><b>Page not found</b></h1>", HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "%s page not found", req->uri);
    return ESP_OK;
}

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

    // Open the namespace in read-write mode. If we try to open it the first time
    // in READONLY mode it will return and error.
    nvs_handle_t paramHandle;
    ESP_ERROR_CHECK(nvs_open("esplib", NVS_READWRITE, &paramHandle));    
    nvs_close(paramHandle);
    
    _eventGroup = xEventGroupCreate();

    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    startWebServer();
    httpd_register_err_handler(_server, HTTPD_404_NOT_FOUND, connectedHTTP404ErrorHandler);
}

void
IDFWiFiPortal::setConfigHandler(HandlerCB f)
{
    _configHandler = f;
}

esp_err_t
IDFWiFiPortal::thunkHandler(httpd_req_t* req)
{
    HandlerThunk* thunk = reinterpret_cast<HandlerThunk*>(req->user_ctx);
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(thunk->_portal);
    self->_activeRequest = req;
    
    std::string endpointTail;
    std::string uri = std::string(req->uri);
    
    // If this is a wildcard match, save the tail for the callback
    if (thunk->_endpoint == uri.substr(0, thunk->_endpoint.length())) {
        endpointTail = uri.substr(thunk->_endpoint.length()).c_str();

        // If this is a wildcard match the query string will still be at the end
        // of the endpointTail. It is also in the queryString so we can just toss it.
        std::string::size_type n = endpointTail.find('?');
        if (n != std::string::npos) {
            endpointTail.erase(n);
        }
    }
    
    self->_parser = std::make_unique<HTTPParser>();

    // Get the arg string
    size_t queryURLLen = httpd_req_get_url_query_len(req) + 1;

    if (queryURLLen > 1) {    
        // Allocate temporary buffer to store the parameter
        std::vector<char> buf(queryURLLen);
        if (httpd_req_get_url_query_str(req, buf.data(), queryURLLen) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to extract query URL");
        } else {
            std::string args(buf.data(), queryURLLen);
            self->_parser->parseQuery(HTTPParser::urlDecode(args));
        }
    }
    
    if (req->method == HTTP_POST) {
        std::string contentType = self->getHTTPHeader("Content-Type");
        if (contentType.empty()) {
            self->_parser->setErrorResponse(501, "no Content-Type");
        } else {
            std::vector<std::string> multipart = HTTPParser::parseFormData(contentType);
            if (multipart[0] != "multipart/form-data" || multipart[1] != "boundary") {
                // This is not a multipart, Handle it normally
                thunk->_handler(thunk->_portal, endpointTail.c_str());
            } else {
                std::string lengthString = self->getHTTPHeader("Content-Length");
                size_t contentLength = std::stoi(lengthString);
                self->_parser->parseMultipart(contentLength, multipart[2],
                    [thunk](const char*)
                    {
                        thunk->_handler(thunk->_portal, nullptr);
                    },
                    [req](uint8_t* buf, size_t size) -> ssize_t
                    {
                        return httpd_req_recv(req, reinterpret_cast<char*>(buf), size);
                    }
                );
            }
        }
    } else {
        thunk->_handler(thunk->_portal, endpointTail.c_str());
    }

    if (self->_parser->errorCode()) {
        self->sendHTTPResponse(self->_parser->errorCode(), "text/plain", self->_parser->errorReason().c_str());
        System::logE(TAG, "HTTP parser error (%d):%s\n", self->_parser->errorCode(), self->_parser->errorReason().c_str());
    }
    
    self->_parser.reset();
    self->_activeRequest = nullptr;
    
    return ESP_OK;
}

int32_t
IDFWiFiPortal::addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB)
{
    if (!_server) {
        System::logE(TAG, "can't add HTTP handler for '%s', server not initialized", endpoint);
        return -1;
    }
    
    httpd_uri_t uri = {
        .uri = endpoint,
        .method = (method == HTTPMethod::Post) ? HTTP_POST : HTTP_GET,
        .handler = thunkHandler,
        .user_ctx = new HandlerThunk(requestCB, this, endpoint)
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(_server, &uri));
    return 0;
}

void
IDFWiFiPortal::addStaticHTTPHandler(const char *uri, const char *path)
{
    addHTTPHandler((std::string(uri) + "/*").c_str(), HTTPMethod::Get, [uri, path](WiFiPortal* p, const char*) {
        IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(p);
        
        std::string endpoint(uri);
        std::string filePath(self->_activeRequest->uri);
        filePath = filePath.substr(endpoint.length());
        if (filePath.back() == '?') {
            filePath.pop_back();
        }

        std::string f(path);
        f += filePath;
    
        if (!self->_wfs || !self->_wfs->exists(f.c_str())) {
            httpd_resp_send(self->_activeRequest, "<h1><b>Page not found</b></h1>", HTTPD_RESP_USE_STRLEN);
            ESP_LOGI(TAG, "%s page not found", self->_activeRequest->uri);
        } else {
            fs::File file = self->_wfs->open(f.c_str(), "r");
            self->streamHTTPResponse(file, HTTPParser::suffixToMimeType(f).c_str(), false);
            file.close();
        }
    });
}

bool
IDFWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    scanNetworks();

    getNVSParam("wifi_ssid", _ssid);
    getNVSParam("wifi_pass", _pass);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this, nullptr));
        
    if (!_ssid.empty()) {
        // Connect to WiFi
        ESP_LOGI(TAG, "Found credentials, connecting to '%s'", _ssid.c_str());
        
        esp_netif_t *netif = esp_netif_create_default_wifi_sta();
        if (!getHostname().empty()) {
            ESP_LOGI(TAG, "Setting hostname to '%s'", getHostname().c_str());
            esp_netif_set_hostname(netif, getHostname().c_str());

            ESP_ERROR_CHECK(mdns_init());
            ESP_LOGI(TAG, "Setting mdns hostname to '%s'", getHostname().c_str());
            ESP_ERROR_CHECK(mdns_hostname_set(getHostname().c_str()));
        }
        
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
        System::delay(2000);

        EventBits_t bits = xEventGroupWaitBits(_eventGroup,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(30000));
        if (bits & WIFI_CONNECTED_BIT) {
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

// HTTP Error (404) Handler for the captive portal - Redirects all requests to the root page
static esp_err_t portalHTTP404ErrorHandler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
    sendHTTPResponse(code, mimetype, data, strlen(data), false);
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
    if (!_activeRequest) {
        ESP_LOGE(TAG, "Can't send HTTP response. No active request.");
        return;
    }
    
    ESP_ERROR_CHECK(httpd_resp_set_type(_activeRequest, mimetype ?: "text/plain"));
    ESP_ERROR_CHECK(httpd_resp_set_hdr(_activeRequest, "Content-Length", std::to_string(length).c_str()));
    if (gzip) {
        ESP_ERROR_CHECK(httpd_resp_set_hdr(_activeRequest, "Content-Encoding", "gzip"));
    }
    httpd_resp_send(_activeRequest, data, length);
}    

void
IDFWiFiPortal::streamHTTPResponse(fs::File& file, const char* mimetype, bool attach)
{
    // For now assume this is a file download. So set Content-Disposition
    std::string disp = attach ? "attachment" : "inline";
    disp += "; filename=\"";
    disp += file.name();
    disp += "\"";
    
    ESP_ERROR_CHECK(httpd_resp_set_hdr(_activeRequest, "Content-Disposition", disp.c_str()));
    ESP_ERROR_CHECK(httpd_resp_set_type(_activeRequest, mimetype ?: "text/plain"));
    
    uint8_t buf[256];

    while (true) {
        int size = file.read(buf, 256);
        if (size < 0) {
            printf("**** Error reading file\n");
            if (_parser) {
                _parser->setErrorResponse(404, "Error reading file");
            }
            break;
        } else if (size == 0) {
            break;
        }
        
        if (httpd_resp_send_chunk(_activeRequest, reinterpret_cast<char*>(buf), size) != ESP_OK) {
            ESP_LOGE(TAG, "File sending failed!");
            
            httpd_resp_sendstr_chunk(_activeRequest, NULL);
            httpd_resp_send_err(_activeRequest, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return;
        }

        if (size < 256) {
            break;
        }
    }
    
    httpd_resp_set_hdr(_activeRequest, "Connection", "close");
    httpd_resp_send_chunk(_activeRequest, NULL, 0);
}

WiFiPortal::HTTPUploadStatus
IDFWiFiPortal::httpUploadStatus() const
{
    return _parser ? _parser->httpUploadStatus() : HTTPUploadStatus::None;
}

std::string
IDFWiFiPortal::httpUploadFilename() const
{
    return _parser ? _parser->httpUploadFilename() : "";
}

size_t
IDFWiFiPortal::httpUploadTotalSize() const
{
    return _parser ? _parser->httpUploadTotalSize() : 0;
}

size_t
IDFWiFiPortal::httpUploadCurrentSize() const
{
    return _parser ? _parser->httpUploadCurrentSize() : 0;
}

const uint8_t*
IDFWiFiPortal::httpUploadBuffer() const
{
    return _parser ? _parser->httpUploadBuffer() : nullptr;
}

int
IDFWiFiPortal::receiveHTTPResponse(char* buf, size_t size)
{
    return httpd_req_recv(_activeRequest, buf, size);
}

std::string
IDFWiFiPortal::getHTTPArg(const char* name)
{
    if (!_activeRequest || !_parser) {
        return "";
    }

    return _parser->getHTTPArg(name);
}

void
IDFWiFiPortal::parseQuery(const char* queryString)
{
    if (_activeRequest && _parser) {
        _parser->parseQuery(queryString);
    }
}

std::string
IDFWiFiPortal::getHTTPHeader(const char* name)
{
    if (!_activeRequest) {
        return "";
    }
    
    size_t size = httpd_req_get_hdr_value_len(_activeRequest, name) + 1;

    if(size < 2) {
        return "";
    }

    // Allocate temporary buffer to store the parameter
    std::vector<char> buf(size);
    if (httpd_req_get_hdr_value_str(_activeRequest, name, buf.data(), size) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to extract get header");
        return "";
    }
    return std::string(buf.data(), size - 1);
}

std::string
IDFWiFiPortal::getCPUModel() const
{
    // Get CPU Model
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    std::string cpuModel;

    switch (chipInfo.model) {
        case CHIP_ESP32:
            cpuModel = "ESP32";
            break;
        case CHIP_ESP32S2:
            cpuModel = "ESP32s2";
            break;
        case CHIP_ESP32S3:
            cpuModel = "ESP32s3";
            break;
        case CHIP_ESP32C3:
            cpuModel = "ESP32c3";
            break;
        case CHIP_ESP32C2:
            cpuModel = "ESP32c2";
            break;
        case CHIP_ESP32C6:
            cpuModel = "ESP32c6";
            break;
        case CHIP_ESP32H2:
            cpuModel = "ESP32h2";
            break;
        case CHIP_ESP32P4:
            cpuModel = "ESP32p4";
            break;
        case CHIP_POSIX_LINUX:
            cpuModel = "Sim";
            break;
        default:
            cpuModel = "Unknown";
            break;
    }
    return cpuModel;
}

uint32_t
IDFWiFiPortal::getCPUFrequency() const
{
    rtc_cpu_freq_config_t freq_config;
    rtc_clk_cpu_freq_get_config(&freq_config);
    return freq_config.freq_mhz;
}

float
IDFWiFiPortal::getCPUTemperature() const
{
    temperature_sensor_handle_t tempSensor = nullptr;
    temperature_sensor_config_t tempSensorConfig = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&tempSensorConfig, &tempSensor));
    ESP_ERROR_CHECK(temperature_sensor_enable(tempSensor));
    float cpuTemp;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(tempSensor, &cpuTemp));
    ESP_ERROR_CHECK(temperature_sensor_disable(tempSensor));
    ESP_ERROR_CHECK(temperature_sensor_uninstall(tempSensor));
    return cpuTemp;
}

uint32_t
IDFWiFiPortal::getCPUUptime() const
{
    return esp_timer_get_time() / 1000000;
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
IDFWiFiPortal::getNVSParam(const char* id, std::string& value) const
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
        err = ESP_ERR_NOT_FOUND;
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

static void dhcpSetCaptivePortalURL()
{
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    char* captiveportal_uri = (char*) malloc(32 * sizeof(char));
    assert(captiveportal_uri && "Failed to allocate captiveportal_uri");
    strcpy(captiveportal_uri, "http://");
    strcat(captiveportal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captiveportal_uri, strlen(captiveportal_uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(netif));
}

void
IDFWiFiPortal::startProvisioning()
{
    _provisioning = true;
    
    if (_server) {
        httpd_register_err_handler(_server, HTTPD_404_NOT_FOUND, portalHTTP404ErrorHandler);
    } else {
        System::logE(TAG, "HTTP server not started, can't register provisioning error handler");
    }
    
    // Call the config handler
    if (_configHandler) {
        _configHandler(this, nullptr);
    }
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
    System::delay(2000);

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    ESP_LOGI(TAG, "AP started. SSID: '%s', Connect to http://" IPSTR, PROV_AP_SSID, IP2STR(&ip_info.ip));

    dhcpSetCaptivePortalURL();

    // Start the DNS server that will redirect all queries to the softAP IP
    dns_server_config_t config = {
        .num_of_entries = 1,
        .item = {
            { 
                .name = "*",    
                .if_key = "WIFI_AP_DEF",
                .ip = 0
            }
        }
    };
    start_dns_server(&config);

    // Don't ever return. When we get valid credentials we will reset and try again
    while (true) {
        System::delay(100);
    }
}

void
IDFWiFiPortal::startWebServer()
{
    if (_server) {
        System::logW(TAG, "Web Server already started");
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 32;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&_server, &config);
    if (err == ESP_OK) {
        System::logI(TAG, "Web Server started");
    } else {
        System::logE(TAG, "failed to start Web Server (%s)", esp_err_to_name(err));
    }
}

void
IDFWiFiPortal::scanNetworks()
{
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        System::logE(TAG, "failed to create default STA");
    } else {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        esp_wifi_set_mode(WIFI_MODE_STA);
        ESP_ERROR_CHECK(esp_wifi_start());
        
        esp_err_t err = esp_wifi_scan_start(nullptr, true);
        if (err == ESP_OK) {
            System::logI(TAG, "scanNetworks succeeded");
        } else {
            System::logE(TAG, "scanNetworks failed (%s)", esp_err_to_name(err));
            return;
        }

        uint16_t apCount = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
        System::logI(TAG, "Total APs scanned = %u", apCount);
        
        if (apCount) {
            std::vector<wifi_ap_record_t> apInfo(apCount);
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, apInfo.data()));
            
            // Copy the ssids to list
            _knownNetworks.clear();
            for (auto& it : apInfo) {
                _knownNetworks.push_back({ std::string(reinterpret_cast<const char*>(it.ssid)), it.rssi, it.authmode == WIFI_AUTH_OPEN });
            }

            // Get rid of duplicates
            std::sort(_knownNetworks.begin(), _knownNetworks.end());
            _knownNetworks.erase(std::unique(_knownNetworks.begin(), _knownNetworks.end()), _knownNetworks.end());
        
            for (auto& it : _knownNetworks) {
                System::logI(TAG, "********** ssid='%s', rssi=%d, open=%s", it.ssid.c_str(), int(it.rssi), it.open ? "true" : "false");
            }
        }
    }
    
    // Shut down WiFi so the STA or AP can start it up again
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
        snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&event->ip_info.netmask));
        self->_currentMSK = ipStr;
        snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&event->ip_info.gw));
        self->_currentGW = ipStr;

        // Get DNS
        esp_netif_dns_info_t dnsInfo;
        esp_err_t err = esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dnsInfo);
        if (err == ESP_OK) {
            snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&dnsInfo.ip.u_addr.ip4));
            self->_currentDNS = ipStr;
        } else {
            self->_currentDNS = "unknown";
        }
        
        self->_retryNum = 0;
        self->_isConnected = true;
        xEventGroupSetBits(self->_eventGroup, WIFI_CONNECTED_BIT);
    }
}

void
IDFWiFiPortal::otaUpdate()
{
    bool finished = false;
    
    switch(httpUploadStatus()) {
        case HTTPUploadStatus::Start: {
            std::string f = HTTPParser::urlDecode(getHTTPArg("path")) + "/" + httpUploadFilename();
            _otaUpdateAborted = false;
            size_t length = _parser->httpUploadContentLength();

            _updatePartition = esp_ota_get_next_update_partition(nullptr);
            if (!_updatePartition) {
                System::logW(TAG, "ota Update: no updatePartition");
                _otaUpdateAborted = true;
                finished = true;
            } else {
                System::logI(TAG, "Writing to partition subtype %d at offset 0x%04x", int(_updatePartition->subtype), int(_updatePartition->address));
                esp_err_t err = esp_ota_begin(_updatePartition, length ? length : OTA_WITH_SEQUENTIAL_WRITES, &_otaUpdateHandle);
                if (err != ESP_OK) {
                    System::logE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                    _otaUpdateAborted = true;
                    finished = true;
                } else {
                    System::logI(TAG, "esp_ota_begin succeeded");
                }
            }
            break;
        }
        case HTTPUploadStatus::Write:
            if (!_otaUpdateAborted) {
                // Write the received chunk to the file
                size_t currentSize = httpUploadCurrentSize();

                esp_err_t err = esp_ota_write(_otaUpdateHandle, reinterpret_cast<const void *>(httpUploadBuffer()), currentSize);
                if (err != ESP_OK) {
                    System::logE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                    _otaUpdateAborted = true;
                    finished = true;
                    esp_ota_abort(_otaUpdateHandle);
                }
            }
            break;
        case WiFiPortal::HTTPUploadStatus::End: {
            finished = true;

            esp_err_t err = esp_ota_end(_otaUpdateHandle);
            if (err != ESP_OK) {
                System::logE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
                _otaUpdateAborted = true;
                esp_ota_abort(_otaUpdateHandle);
            } else {
                err = esp_ota_set_boot_partition(_updatePartition);
                if (err != ESP_OK) {
                    System::logE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
                    _otaUpdateAborted = true;
                    esp_ota_abort(_otaUpdateHandle);
                }
            }
            break;
        }
        case WiFiPortal::HTTPUploadStatus::Aborted:
            System::logW(TAG, "handleUpload: Upload Aborted");
            _otaUpdateAborted = true;
            finished = true;
            esp_ota_abort(_otaUpdateHandle);
            break;
        case WiFiPortal::HTTPUploadStatus::None:
            System::logW(TAG, "Invalid ota update status");
            _otaUpdateAborted = true;
            finished = true;
            break;
    }
    
    if (finished) {
        if (_otaUpdateAborted) {
            sendHTTPResponse(500, "text/plain", "Upload Aborted");
            System::logW(TAG, "Reply sent: ota Update Aborted");
        } else if (httpUploadTotalSize() > 0) { // Check if any bytes were received
            sendHTTPResponse(200, "text/html", "<h1>OTA Update Successful. Restarting...</h1>");
            System::logI(TAG, "Reply sent: Successful ota update, restarting...\n");
            System::delay(2000);
            System::restart();
        } else {
            // This might happen if the file was empty or write failed early
            sendHTTPResponse(500, "text/plain", "ota Update Failed or Empty File");
            System::logE(TAG, "Reply sent: ota Update Failed/Empty\n");
        }
    }
}


#endif
