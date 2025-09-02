/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "System.h"

#if defined ARDUINO

//**************************************************************************
//                                   Arduino
//**************************************************************************

#include "Preferences.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

Preferences _prefs;

void
System::begin()
{
    _prefs.begin("ESPLib");
}

String
System::getPrefString(const char* id)
{
    return _prefs.getString(id);
}

void
System::putPrefString(const char* id, const char* value)
{
    _prefs.putString(id, value);
}

String
System::localIP()
{
    return WiFi.localIP().toString();
}

void
System::restart()
{
    ESP.restart();
}    

#elif defined ESP_PLATFORM

//**************************************************************************
//                                   esp-idf
//**************************************************************************

#include "nvs_flash.h"
#include "nvs.h"

void
System::begin()
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
System::getPrefString(const char* id)
{
    return "";
}

void
System::putPrefString(const char* id, const char* value)
{
}

String
System::localIP()
{
//    esp_netif_ip_info_t ip_info;
//    esp_netif_get_ip_info(IP_EVENT_STA_GOT_IP,&ip_info);
//    char buf[20];
//    sprintf("IPSTR", IP2STR(&ip_info.ip));
//    return buf;
    return "";
}

void
System::restart()
{
    esp_restart();
}    

Ticker::~Ticker()
{
    detach();
}

void
Ticker::_attach_us(uint64_t micros, bool repeat, callback_with_arg_t callback, void *arg)
{
    esp_timer_create_args_t _timerConfig;
    _timerConfig.arg = reinterpret_cast<void *>(arg);
    _timerConfig.callback = callback;
    _timerConfig.dispatch_method = ESP_TIMER_TASK;
    _timerConfig.name = "Ticker";
    if (_timer) {
        esp_timer_stop(_timer);
        esp_timer_delete(_timer);
    }
    esp_timer_create(&_timerConfig, &_timer);
    if (repeat) {
        esp_timer_start_periodic(_timer, micros);
    } else {
        esp_timer_start_once(_timer, micros);
    }
}

void
Ticker::detach()
{
    if (_timer) {
        esp_timer_stop(_timer);
        esp_timer_delete(_timer);
        _timer = nullptr;
        _cb = nullptr;
    }
}

bool
Ticker::active() const
{
    if (!_timer) {
        return false;
    }
    return esp_timer_is_active(_timer);
}

void
Ticker::_static_callback(void *arg)
{
    Ticker *_this = reinterpret_cast<Ticker *>(arg);
    if (_this && _this->_cb) {
        _this->_cb();
    }
}

#else // Mac

//**************************************************************************
//                                   Mac
//**************************************************************************

void
System::begin()
{
}

String
System::getPrefString(const char* id)
{
    return "";
}

void
System::putPrefString(const char* id, const char* value)
{
}

String
System::localIP()
{
    return "0.0.0.0";
}

void
System::restart()
{
    printf("***** RESTART *****\n");
}    

#endif

//**************************************************************************
//                         Cross-platform System methods
//**************************************************************************

void
System::initParams()
{
    for (auto& it : _params) {
        const char* id = it.id.c_str();
        String savedValue = getPrefString(id);
        if (savedValue.length() == 0) {
            const char* value = it.value.c_str();
            putPrefString(id, value);
            printf("No '%s' saved. Setting it to default: '%s'\n", id, value);
        } else {
            it.value = savedValue;
            printf("Setting '%s' to saved value: '%s'\n", id, savedValue.c_str());
        }
    } 
}
