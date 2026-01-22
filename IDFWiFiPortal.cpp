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

using namespace mil;

static const char* TAG = "WiFiPortal";
static const char* PROV_AP_SSID = "ESP32-Provisioning";
static const char* PROV_AP_PASS = "password123";
static constexpr uint32_t PROV_AP_MAX_CONN = 4;

static inline std::string q(const std::string& s) { return "\"" + s + "\""; }
static inline std::string jp(const std::string& n, const std::string& v) { return q(n) + ":" + q(v); }

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
    _title = title;
}

void
IDFWiFiPortal::setCustomMenuHTML(const char* html)
{
    _customHTML = html;
}

void
IDFWiFiPortal::setHostname(const char* name)
{
    _hostname = name;
}

void
IDFWiFiPortal::setConfigHandler(HandlerCB f)
{
    printf("*** setConfigHandler not implemented\n");
}

esp_err_t
IDFWiFiPortal::thunkHandler(httpd_req_t* req)
{
    HandlerThunk* thunk = reinterpret_cast<HandlerThunk*>(req->user_ctx);
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(thunk->_portal);
    self->_activeRequest = req;

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
            self->_parser->parseQuery(args);
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
                thunk->_handler(thunk->_portal);
            } else {
                self->_parser->parseMultipart(multipart[2],
                    [thunk]()
                    {
                        thunk->_handler(thunk->_portal);
                    },
                    [req](uint8_t* buf, size_t size) -> ssize_t
                    {
                        return httpd_req_recv(req, reinterpret_cast<char*>(buf), size);
                    }
                );
            }
        }
    } else {
        thunk->_handler(thunk->_portal);
    }

    if (self->_parser->errorCode()) {
        self->sendHTTPResponse(self->_parser->errorCode(), "text/plain", self->_parser->errorReason().c_str());
        printf("***** Error (%d):%s\n", self->_parser->errorCode(), self->_parser->errorReason().c_str());
    }
    
    self->_parser.release();
    self->_activeRequest = nullptr;
    
    return ESP_OK;
}

int32_t
IDFWiFiPortal::addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB)
{
    httpd_uri_t uri = {
        .uri = endpoint,
        .method = (method == HTTPMethod::Post) ? HTTP_POST : HTTP_GET,
        .handler = thunkHandler,
        .user_ctx = new HandlerThunk(requestCB, this)
    };
    httpd_register_uri_handler(_server, &uri);
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
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this, nullptr));

    getParamValue("wifi_ssid", _ssid);
    getParamValue("wifi_pass", _pass);
    
    if (!_ssid.empty()) {
        // Connect to WiFi
        ESP_LOGI(TAG, "Found credentials, connecting to '%s'", _ssid.c_str());
        
        esp_netif_t *netif = esp_netif_create_default_wifi_sta();
        if (!_hostname.empty()) {
            ESP_LOGI(TAG, "Setting hostname to '%s'", _hostname.c_str());
            esp_netif_set_hostname(netif, _hostname.c_str());

            ESP_ERROR_CHECK(mdns_init());
            ESP_LOGI(TAG, "Setting mdns hostname to '%s'", _hostname.c_str());
            ESP_ERROR_CHECK(mdns_hostname_set(_hostname.c_str()));
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

        EventBits_t bits = xEventGroupWaitBits(_eventGroup,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(30000));

        if (bits & WIFI_CONNECTED_BIT) {
            startWebServer(false);
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

// HTTP Error (404) Handler when connected - show a 404 page
static esp_err_t connectedHTTP404ErrorHandler(httpd_req_t *req, httpd_err_code_t err)
{
    ESP_ERROR_CHECK(httpd_resp_set_status(req, "404 Not Found"));
    httpd_resp_send(req, "<h1><b>Page not found</b></h1>", HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "%s page not found", req->uri);
    return ESP_OK;
}

void
IDFWiFiPortal::startWebPortal()
{
    startWebServer(false);
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

    assert(_activeRequest);

    ESP_ERROR_CHECK(httpd_resp_set_type(_activeRequest, mimetype));
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
    ESP_ERROR_CHECK(httpd_resp_set_type(_activeRequest, mimetype));
    
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
    
    ESP_LOGI(TAG, "File sending complete");
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

std::string
IDFWiFiPortal::getHTTPArg(const char* name)
{
    if (!_activeRequest || !_parser) {
        ESP_LOGW(TAG, "getHTTPArg can't get '%s' arg. No parser", name);
        return "";
    }

    std::string s =  _parser->getHTTPArg(name);
    if (s.empty()) {
        ESP_LOGW(TAG, "getHTTPArg '%s' is empty", name);
    }
    return s;
}

const std::string
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

bool
IDFWiFiPortal::addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
{
    std::string value;

    auto entry = _paramMap.find(id);
    if (entry == _paramMap.end()) {
        // New map entry
        _paramMap.insert({ id, { label, maxLength } });
    }
    
    // If we don't have a param entry yet, set it to the default
    if (!getNVSParam(id, value)) {
        setNVSParam(id, defaultValue);
    }
    
    return true;
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

    dhcpSetCaptivePortalURL();

    startWebServer(true);

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
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void
IDFWiFiPortal::startWebServer(bool provision)
{
    if (_server) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;

    if (httpd_start(&_server, &config) == ESP_OK) {
        // If we're provisioning we bring up the captive portal (wifi setup) page.
        // Otherwise we bring up the landing page, which shows buttons for all
        // the functionality you can get to. This page is customizable both by
        // rearranging the buttons and by adding custom html.
        if (provision) {
            addHTTPHandler("/", WiFiPortal::HTTPMethod::Get, provisioningGetHandler);
            httpd_register_err_handler(_server, HTTPD_404_NOT_FOUND, portalHTTP404ErrorHandler);
        } else {
            addHTTPHandler("/", WiFiPortal::HTTPMethod::Get, landingPageHandler);
            addHTTPHandler("/wifi", WiFiPortal::HTTPMethod::Get, provisioningGetHandler);
            httpd_register_err_handler(_server, HTTPD_404_NOT_FOUND, connectedHTTP404ErrorHandler);
        }
        addHTTPHandler("/connect", HTTPMethod::Post, connectPostHandler);
        addHTTPHandler("/reset", WiFiPortal::HTTPMethod::Get, resetGetHandler);
        addHTTPHandler("/get-wifi-setup", WiFiPortal::HTTPMethod::Get, getWifiSetupHandler);
        addHTTPHandler("/get-landing-setup", WiFiPortal::HTTPMethod::Get, getLandingSetupHandler);
        addHTTPHandler("/favicon.ico", WiFiPortal::HTTPMethod::Get, faviconGetHandler);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}

void
IDFWiFiPortal::scanNetworks()
{
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_set_mode(WIFI_MODE_STA);

    ESP_ERROR_CHECK(esp_wifi_start());

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
            _knownNetworks.push_back({ std::string(reinterpret_cast<const char*>(it.ssid)), it.rssi, it.authmode == WIFI_AUTH_OPEN });
        }

        // Get rid of duplicates
        std::sort(_knownNetworks.begin(), _knownNetworks.end());
        _knownNetworks.erase(std::unique(_knownNetworks.begin(), _knownNetworks.end()), _knownNetworks.end());
    
        for (auto& it : _knownNetworks) {
            ESP_LOGI(TAG, "********** ssid='%s', rssi=%d, open=%s", it.ssid.c_str(), int(it.rssi), it.open ? "true" : "false");
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
        self->_retryNum = 0;
        self->_isConnected = true;
        xEventGroupSetBits(self->_eventGroup, WIFI_CONNECTED_BIT);
    }
}

void
IDFWiFiPortal::provisioningGetHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);

    // Send the portal page
    if (self->_wfs) {
        self->_wfs->sendWiFiPage(self);
    }
}

void
IDFWiFiPortal::connectPostHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);
    
    char buf[128];
    int ret = httpd_req_recv(self->_activeRequest, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(self->_activeRequest);
        }
        return;
    }
    buf[ret] = '\0';

    char ssidBuf[33] = {0};
    char passBuf[65] = {0};
    
    char hostnameBuf[65] = {0};
    
    if (httpd_query_key_value(buf, "ssid", ssidBuf, sizeof(ssidBuf)) != ESP_OK) {
        httpd_resp_send_err(self->_activeRequest, HTTPD_400_BAD_REQUEST, "Missing 'ssid' parameter");
        return;
    }
    httpd_query_key_value(buf, "password", passBuf, sizeof(passBuf));
    
    httpd_query_key_value(buf, "hostname", hostnameBuf, sizeof(hostnameBuf));
    
    // Decode the strings
    std::string ssid = HTTPParser::urlDecode(ssidBuf);
    std::string pass = HTTPParser::urlDecode(passBuf);

    std::string hostname = HTTPParser::urlDecode(hostnameBuf);
    
    if (ssid.empty()) {
        self->getNVSParam("wifi_ssid", ssid);
        self->getNVSParam("wifi_pass", pass);
    }
    
    if (hostname.empty()) {
        self->getNVSParam("hostname", hostname);
    }

    std::string response = "<h1>Connecting to '";
    response += ssid;
    response += "'...</h1><p>If successful, the device will connect to the network. If failed, it will remain in provisioning mode.</p>";
    httpd_resp_sendstr(self->_activeRequest, response.c_str());

    self->setNVSParam("wifi_ssid", ssid);
    self->setNVSParam("wifi_pass", pass);

    self->setNVSParam("hostname", hostname);

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

void
IDFWiFiPortal::landingPageHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);

    // Send the portal page
    if (self->_wfs) {
        self->_wfs->sendLandingPage(self);
    }
}

void
IDFWiFiPortal::resetGetHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);
    
    const char* resp_str = "<h1>Credentials Cleared</h1><p>The device will restart and enter provisioning mode.</p>";
    httpd_resp_send(self->_activeRequest, resp_str, HTTPD_RESP_USE_STRLEN);
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    self->resetSettings();
    esp_restart();
}

void
IDFWiFiPortal::getWifiSetupHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);
    
    std::string ssid, hostname;
    self->getNVSParam("wifi_ssid", ssid);
    self->getNVSParam("hostname", hostname);

    std::string response = "{" + jp("ssid", ssid) + "," + jp("hostname", hostname) + ",";
    response += q("knownNetworks") + ":[";

    bool first = true;
    
    for (const auto& it : self->_knownNetworks) {
        if (!first) {
            response += ",";
        } else {
            first = false;
        }
        
        response += "{" + jp("ssid", it.ssid) + ",";
        response += jp("rssi", std::to_string(it.rssi)) + ",";
        response += jp("open", std::string(it.open ? "true" : "false")) + "}";
    }
    response += "]}";

    httpd_resp_send(self->_activeRequest, response.c_str(), HTTPD_RESP_USE_STRLEN);
}

void
IDFWiFiPortal::getLandingSetupHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);

    std::string response = "{";
    response += jp("title", self->_title) + ",";
    response += jp("ssid", self->_ssid) + ",";
    response += jp("ip", self->_currentIP) + ",";
    response += jp("hostname", self->_hostname) + ",";
    response += jp("customMenuHTML", self->_customHTML);
    response += "}";
    httpd_resp_send(self->_activeRequest, response.c_str(), HTTPD_RESP_USE_STRLEN);
}

void
IDFWiFiPortal::faviconGetHandler(WiFiPortal* portal)
{
    IDFWiFiPortal* self = reinterpret_cast<IDFWiFiPortal*>(portal);

    httpd_resp_set_status(self->_activeRequest, "204 No Content");
    httpd_resp_send(self->_activeRequest, NULL, 0);
}

#endif
