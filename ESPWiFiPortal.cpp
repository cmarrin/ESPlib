/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "ESPWiFiPortal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

#include "HTTPParser.h"
#include "WebFileSystem.h"

using namespace mil;

static const char *TAG = "WiFiPortal";
static const char* PROV_AP_SSID = "ESP32-Provisioning";

void
ESPWiFiPortal::begin(WebFileSystem* wfs)
{
    _wfs = wfs;
    _prefs.begin("ESPLib");
}

void
ESPWiFiPortal::resetSettings()
{
    eraseNVSParam("wifi_ssid");
    eraseNVSParam("wifi_pass");
}

void
ESPWiFiPortal::setConfigHandler(HandlerCB f)
{
    _configHandler = f;
}

int32_t
ESPWiFiPortal::addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB)
{
    if (!_server) {
        System::logE(TAG, "addHTTPHandler: server not running");
        return -1;
    }
    
    if (method == HTTPMethod::Post) {
        _server->on(endpoint, HTTP_POST, [this]() { _server->send(200, "text/plain", ""); },  [this, requestCB]() { requestCB(this); });
    } else {
        _server->on(endpoint, [this, requestCB]() { requestCB(this); });
    }
    return 0;
}

void
ESPWiFiPortal::addStaticHTTPHandler(const char *uri, const char *path)
{
    _server->serveStatic(uri, LittleFS, path);
}

void
ESPWiFiPortal::scanNetworks()
{
    _knownNetworks.clear();
    int n = WiFi.scanNetworks();
    if (n == 0) {
        System::logI(TAG, "No networks found during scan");
    } else {
        for (int i = 0; i < n; ++i) {
            _knownNetworks.emplace_back(std::string(WiFi.SSID(i).c_str()), int8_t(WiFi.RSSI(i)), WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        }

        // Get rid of duplicates
        std::sort(_knownNetworks.begin(), _knownNetworks.end());
        _knownNetworks.erase(std::unique(_knownNetworks.begin(), _knownNetworks.end()), _knownNetworks.end());
    
        for (auto& it : _knownNetworks) {
            ESP_LOGI(TAG, "ssid='%s', rssi=%d, open=%s", it.ssid.c_str(), int(it.rssi), it.open ? "true" : "false");
        }
    }
    WiFi.scanDelete();
}

bool
ESPWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    scanNetworks();

    getNVSParam("wifi_ssid", _ssid);
    getNVSParam("wifi_pass", _pass);
    
    if (!_ssid.empty()) {
        // Try connecting
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);
        
        System::logI(TAG, "Found credentials, connecting to '%s'", _ssid.c_str());
        
        int result = WiFi.WaitForConnectResult();
        if (result != WL_CONNECTED) {
            System::logI(TAG, "Failed to connect with stored credentials:%d", result);
        } else {
            System::logI(TAG, "Connection succeeded", result);
            return true;
        }
    } else {
        System::logI(TAG, "Credentials not found");
    }
    
    System::logI(TAG, "Starting captive portal");
    startProvisioning();
    return true;
}

void
ESPWiFiPortal::startProvisioning()
{
    WiFi.mode(WIFI_AP);

    WiFi.softAP(PROV_AP_SSID);
    System::logI(TAG, "AP IP: %s", WiFi.softAPIP().toString().c_str());

    DNSServer dnsServer;
    dnsServer.start(53, "*", WiFi.softAPIP());
    startWebServer(true);
}

void
ESPWiFiPortal::connectHandler()
{
    if (!_server->hasArg("ssid") || !_server->hasArg("password")) {
        sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"ssid and password\"}");
    }
    
    std::string ssid = HTTPParser::urlDecode(_server->arg("ssid").c_str());
    std::string pass = HTTPParser::urlDecode(_server->arg("password").c_str());
    std::string hostname = HTTPParser::urlDecode(_server->arg("hostname").c_str());

    if (ssid.empty()) {
        getNVSParam("wifi_ssid", ssid);
        getNVSParam("wifi_pass", pass);
    }
    
    if (hostname.empty()) {
        getNVSParam("hostname", hostname);
    }

    std::string response = "<h1>Connecting to '";
    response += ssid;
    response += "'...</h1><p>If successful, the device will connect to the network. If failed, it will remain in provisioning mode.</p>";
    sendHTTPResponse(400, "text/html", response.c_str());

    _ssid = ssid;
    _pass = _pass;
    _hostname = hostname;
    setNVSParam("wifi_ssid", _ssid);
    setNVSParam("wifi_pass", _pass);
    setNVSParam("hostname", _hostname);
    System::restart();
}

void
ESPWiFiPortal::redirectRoot()
{
    _server->send(200,"text/html", "<html><body><script>location.href='/'</script></body></html>");
}

void
ESPWiFiPortal::startWebServer(bool provision)
{
    if (_server) {
        return;
    }

    if (provision) {
        addHTTPHandler("/", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { _wfs->sendWiFiPage(this); });
        addHTTPHandler("/generate_204", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { redirectRoot(); });
        addHTTPHandler("/hotspot-detect.html", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { redirectRoot(); });
        addHTTPHandler("/ncsi.txt", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*){ sendHTTPResponse(200,"text/plain","Microsoft NCSI"); });
        addHTTPHandler("/connecttest.txt", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*){ sendHTTPResponse(200,"text/plain",""); });
        addHTTPHandler("/fwlink", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { redirectRoot(); });
        _server->onNotFound([this]() { _wfs->sendWiFiPage(this); });
    } else {
        addHTTPHandler("/wifi", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { _wfs->sendWiFiPage(this); });
    }
    addHTTPHandler("/connect", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { connectHandler(); });
    addHTTPHandler("/restart", HTTPMethod::Get, [this](WiFiPortal*) { restartHandler(); });
    addHTTPHandler("/reset", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { resetHandler(); });
    addHTTPHandler("/get-wifi-setup", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*) { getWifiSetupHandler(); });
    addHTTPHandler("/favicon.ico", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal*)
    {
        sendHTTPResponse(204, "text/plain", "No Content");
    });

    if (!getHostname().empty()) {
        // Setup mDNS
        if (!MDNS.begin(_hostname.c_str())) {
            System::logE(TAG, "Error setting up MDNS responder for hostname '%s'", _hostname.c_str());
        }
    }

    _server->begin();
}

static std::string makeRestartPage(const char* text)
{
    // Send a page that shows the text string for 3 seconds and then goes back to the landing page.
    // Assuming it has already restarted going to that page will fail but when you refresh it
    // will show the landing page rather than restarting again!
    const char* response1 =
        "<html>\n"
            "<body>\n";
    const char* response2 =
                "<script>\n"
                    "let timer = setTimeout(function() {\n"
                        "window.location='http://example.com'\n"
                    "}, 3000);\n"
                "</script>\n"
            "</body>\n"
        "</html>\n";

    return std::string(response1) + text + response2;
}

void
ESPWiFiPortal::restartHandler()
{
    sendHTTPResponse(200, "text/html", makeRestartPage("<h1>Restarting...</h1>").c_str());
    delay(2000);
    System::restart();
}

void
ESPWiFiPortal::resetHandler()
{
    sendHTTPResponse(200, "text/html", makeRestartPage("<h1>Credentials Cleared</h1><p>The device will restart and enter provisioning mode.</p>").c_str());
    resetSettings();
    delay(2000);
    System::restart();
}

void
ESPWiFiPortal::getWifiSetupHandler()
{
    std::string ssid, hostname;
    getNVSParam("wifi_ssid", ssid);
    getNVSParam("hostname", hostname);

    std::string response = "{" + WebFileSystem::jsonParam("ssid", ssid) + "," + WebFileSystem::jsonParam("hostname", hostname) + ",";
    response += WebFileSystem::quote("knownNetworks") + ":[";

    bool first = true;
    
    for (const auto& it : _knownNetworks) {
        if (!first) {
            response += ",";
        } else {
            first = false;
        }
        
        response += "{" + WebFileSystem::jsonParam("ssid", it.ssid) + ",";
        response += WebFileSystem::jsonParam("rssi", std::to_string(it.rssi)) + ",";
        response += WebFileSystem::jsonParam("open", std::string(it.open ? "true" : "false")) + "}";
    }
    response += "]}";

    sendHTTPResponse(200, "application/json", response.c_str());
}

void
ESPWiFiPortal::process()
{
    _server->handleClient();
}

std::string
ESPWiFiPortal::getIP()
{
    return ""; //WiFi.localIP().toString().c_str();
}

const char*
ESPWiFiPortal::getSSID()
{
    return "";
}

void
ESPWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
    _server->send(code, mimetype, data);
}

void
ESPWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
    if (gzip) {
        _server->sendHeader("Content-Encoding", "gzip", true);
    }
    
    _server->setContentLength(length);
    _server->send(code, mimetype);
    _server->sendContent(data, length);
}    

void
ESPWiFiPortal::streamHTTPResponse(File& file, const char* mimetype, bool attach)
{
    // For now assume this is a file download. So set Content-Disposition
    std::string disp = attach ? "attachment" : "inline";
    disp += "; filename=\"";
    disp += file.name();
    disp += "\"";
    _server->sendHeader("Content-Disposition", disp.c_str(), true);
    _server->streamFile(file, mimetype);
}

WiFiPortal::HTTPUploadStatus
ESPWiFiPortal::httpUploadStatus() const
{
    switch(_server->upload().status) {
        default: return HTTPUploadStatus::None;
        case UPLOAD_FILE_START: return HTTPUploadStatus::Start;
        case UPLOAD_FILE_WRITE: return HTTPUploadStatus::Write;
        case UPLOAD_FILE_END: return HTTPUploadStatus::End;
        case UPLOAD_FILE_ABORTED: return HTTPUploadStatus::Aborted;
    }
}

std::string
ESPWiFiPortal::httpUploadFilename() const
{
    return _server->upload().filename.c_str();
}

size_t
ESPWiFiPortal::httpUploadTotalSize() const
{
    return _server->upload().totalSize;
}

size_t
ESPWiFiPortal::httpUploadCurrentSize() const
{
    return _server->upload().currentSize;
}

const uint8_t*
ESPWiFiPortal::httpUploadBuffer() const
{
    return _server->upload().buf;
}

std::string
ESPWiFiPortal::getHTTPArg(const char* name)
{
    return _server->arg(name).c_str();
}

bool
ESPWiFiPortal::hasHTTPArg(const char* name)
{
    return !getHTTPArg(name).empty();
}

std::string
ESPWiFiPortal::getCPUModel() const
{
    return ESP.getChipModel();
}

uint32_t
ESPWiFiPortal::getCPUFrequency() const
{
    return ESP.getCpuFreqMHz();
}

float
ESPWiFiPortal::getCPUTemperature() const
{
    return temperatureRead();
}

uint32_t
ESPWiFiPortal::getCPUUptime() const
{
    return millis() / 1000;
}

void
ESPWiFiPortal::setNVSParam(const char* id, const std::string& value)
{
    _prefs.putString(id, value.c_str());
}

bool
ESPWiFiPortal::getNVSParam(const char* id, std::string& value) const
{
    // The Arduino Preferences API stupidly doesn't mark its getter as const
    // so we have to do it for them.
    ESPWiFiPortal* self = const_cast<ESPWiFiPortal*>(this);

    if (!self->_prefs.isKey(id)) {
        return false;
    }
    value = self->_prefs.getString(id).c_str();
    return true;
}

void
ESPWiFiPortal::eraseNVSParam(const char* id)
{
    _prefs.remove(id);
}
