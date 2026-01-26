/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include <string>

// System level functions with specializations for each platform

#if defined ARDUINO
#include <Ticker.h>
#elif defined ESP_PLATFORM

//**************************************************************************
//                                   esp-idf
//**************************************************************************

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

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
    // NeoPixel
    
    enum class GPIOPinMode { Output, Input, InputWithPullup };
    static void gpioSetPinMode(uint8_t pin, GPIOPinMode mode);
    static void gpioWritePin(uint8_t pin, bool state);
    static bool gpioReadPin(uint8_t pin);
    static uint32_t gpioReadAnalog(uint8_t pin);
    
    static void restart();
    
    static std::string vformat(const char* fmt, va_list);

    static std::string format(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string s = vformat(fmt, args);
        va_end(args);
        return s;
    }
    
    static void logV(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('V', tag, fmt, args);
        va_end(args);
    }
    
    static void logD(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('D', tag, fmt, args);
        va_end(args);
    }
    
    static void logI(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('I', tag, fmt, args);
        va_end(args);
    }
    
    static void logW(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('W', tag, fmt, args);
        va_end(args);
    }
    
    static void logE(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('E', tag, fmt, args);
        va_end(args);
    }

private:
    static void log(char type, const char* tag, const char* fmt, va_list args)
    {
        printf("%c %s: %s\n", type, tag, vformat(fmt, args).c_str());
    }
};
