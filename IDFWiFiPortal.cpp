/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "IDFWiFiPortal.h"

#include "nvs_flash.h"
#include "nvs.h"

void
WiFiPortal::begin()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

String
WiFiPortal::getPrefString(const char* id)
{
    return "";
}

void
WiFiPortal::putPrefString(const char* id, const char* value)
{
}

String
WiFiPortal::localIP()
{
//    esp_netif_ip_info_t ip_info;
//    esp_netif_get_ip_info(IP_EVENT_STA_GOT_IP,&ip_info);
//    char buf[20];
//    sprintf("IPSTR", IP2STR(&ip_info.ip));
//    return buf;
    return String();
}

#endif
