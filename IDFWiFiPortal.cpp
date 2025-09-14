/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "IDFWiFiPortal.h"

#include <esp_wifi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include <netdb.h>
#include "esp_netif_ip_addr.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_manager.h"

void
IDFWiFiPortal::begin()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void
IDFWiFiPortal::resetSettings()
{
}

void
IDFWiFiPortal::setTitle(String title)
{
}

void
IDFWiFiPortal::setMenu(std::vector<const char*>& menu)
{
}

void
IDFWiFiPortal::setDarkMode(bool)
{
}

String
IDFWiFiPortal::getPrefString(const char* id)
{
    return "";
}

void
IDFWiFiPortal::putPrefString(const char* id, const char* value)
{
}

void
IDFWiFiPortal::setCustomMenuHTML(const char* html)
{
}

void
IDFWiFiPortal::setHostname(const char*)
{
}

void
IDFWiFiPortal::setConfigHandler(HandlerCB)
{
}

void
IDFWiFiPortal::setShowInfoErase(bool enabled)
{
}

int32_t
IDFWiFiPortal::addHTTPHandler(const char* endpoint, HandleRequestCB)
{
    return -1;
}

bool
IDFWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    wifi_manager_start();
    return true;
}

void
IDFWiFiPortal::process()
{
}

void
IDFWiFiPortal::startWebPortal()
{
}

String
IDFWiFiPortal::localIP()
{
printf("********* localIP enter\n");
    esp_netif_t *intf = esp_netif_get_default_netif();
    if (!intf) {
        printf("Error: no interface to get IP Address\n");
        return "0.0.0.0";
    }
    
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(intf, &ip_info);
    char buf[20];
    sprintf(buf, "%d.%d.%d.%d", IP2STR(&ip_info.ip));
printf("********* localIP str='%s'\n", buf);
    return buf;
}

const char*
IDFWiFiPortal::getSSID()
{
    return "";
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const String& data)
{
}

int
IDFWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    return -1;
}

size_t
IDFWiFiPortal::httpContentLength()
{
    return 0;
}

#endif
