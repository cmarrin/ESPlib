/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

// System level functions with specializations for each platform

#if defined ARDUINO
#include <Ticker.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

class System
{
  public:
    static String localIP() { return WiFi.localIP().toString(); }
    
    // WiFi Setup
    
    // MDNS
    
    // WiFi Manager
    
    // Preferences
    
    // HTTP Client
    
    // HTTP Upload (WebServer, RequestHandler...)
    
    // LED Blinker
    
    // NeoPixel
    
    static void restart() { ESP.restart(); }    
};

#elif defined ESP_PLATFORM

#include <esp_wifi.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "wifi_manager.h"

#include <functional>

class System
{
  public:
    static String localIP()
    {
//        esp_netif_ip_info_t ip_info;
//        esp_netif_get_ip_info(IP_EVENT_STA_GOT_IP,&ip_info);
//        char buf[20];
//        sprintf("IPSTR", IP2STR(&ip_info.ip));
//        return buf;
        return "";
    }

    // WiFi Setup
    
    // MDNS
    
    // WiFi Manager
    
    // Preferences
    
    // HTTP Client
    
    // HTTP Upload (WebServer, RequestHandler...)
        
    // LED Blinker
    
    // NeoPixel
    
    static void restart() { esp_restart(); }
};

class Ticker
{
public:
    using callback_t = std::function<void(void)>;
    
    void once_ms(uint32_t ms, callback_t callback);
    void attach_ms(uint32_t ms, callback_t callback);
    
    // These are functions used by the Max7219 scroll function, so we will not implement them yet
    void detach() { }
    bool active() const { return false; }
};

#else // Mac

#include <functional>
#include <thread>

class System
{
  public:
    static String localIP() { return ""; }

    // WiFi Setup
    
    // MDNS
    
    // WiFi Manager
    
    // Preferences
    
    // HTTP Client
    
    // HTTP Upload (WebServer, RequestHandler...)
    
    // LED Blinker
    
    // NeoPixel
    
    static void restart() { printf("***** RESTART *****\n"); }
};

class Ticker
{
public:
    using callback_t = std::function<void(void)>;
    
    void once_ms(uint32_t ms, callback_t callback)
    {
        _ms = ms;
        _cb = callback;

        std::thread([this]() { 
            std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
            _cb();
        }).detach();
    }
    
    void attach_ms(uint32_t ms, callback_t callback)
    {
        _ms = ms;
        _cb = callback;

        std::thread([this]() { 
            while (true) { 
                auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(_ms);
                _cb();
                std::this_thread::sleep_until(x);
            }
        }).detach();
    }

    // These are functions used by the Max7219 scroll function, so we will not implement them yet
    void detach() { }
    bool active() const { return false; }

private:
    uint32_t _ms = 0;
    callback_t _cb;
};

#endif
