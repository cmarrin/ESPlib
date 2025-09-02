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
#elif defined ESP_PLATFORM

//**************************************************************************
//                                   esp-idf
//**************************************************************************

#include <esp_wifi.h>
#include <esp_netif.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "wifi_manager.h"

#include <functional>

class Ticker
{
public:
    using callback_with_arg_t = void(*)(void*);
    using callback_function_t = std::function<void(void)>;
    
    ~Ticker();
    
    void attach_ms(uint64_t ms, callback_function_t callback)
    {
        _cb = std::move(callback);
        _attach_us(1000ULL * ms, true, _static_callback, this);
    }

    void once_ms(uint32_t ms, callback_function_t callback)
    {
        _cb = std::move(callback);
        _attach_us(1000ULL * ms, false, _static_callback, this);
    }

    // These are functions used by the Max7219 scroll function, so we will not implement them yet
    void detach();
    bool active() const;

private:
    void _attach_us(uint64_t micros, bool repeat, callback_with_arg_t callback, void *arg);
    static void _static_callback(void *arg);

    callback_function_t _cb = nullptr;
    esp_timer_handle_t _timer;
};

#else // Mac

//**************************************************************************
//                                   Mac
//**************************************************************************

#include <functional>
#include <thread>

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
    callback_t _cb = nullptr;
};

#endif

//**************************************************************************
//                            Generic System Class
//**************************************************************************

class System
{
  public:
    void begin();
    
    static String localIP();

    void startNetwork();
    
    // MDNS
    
    // WiFi Manager
    
    // HTTP Client
    
    // HTTP Upload (WebServer, RequestHandler...)
    
    // LED Blinker
    
    // NeoPixel
    
    // The parameter system allows storage and retrieval of string key value pairs. They are
    // persistant, stored in the nvs section of flash.
    void addParam(const char *id, const char *label, const char *defaultValue, uint32_t length)
    {
        _params.push_back({ id, label, defaultValue, length });
    }

    void initParams();
	
    const char* getParamValue(const char* id) const
    {
        for (auto& it : _params) {
            if (strcmp(it.id.c_str(), id) == 0) {
                return it.value.c_str();
            }
        }
        return nullptr;
    }
    
    void saveParams()
    {
        for (auto& it : _params) {
            putPrefString(it.id.c_str(), it.value.c_str());
        }
    }
    static void restart();
    
private:
    String getPrefString(const char* id);
    void putPrefString(const char* id, const char* value);
    
    struct Param
    {
        String id;
        String label;
        String value;
        uint32_t length;
    };

    std::vector<Param> _params;
};
