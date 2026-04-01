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
    void stop();

private:
    void _attach_us(uint64_t micros, bool repeat, callback_with_arg_t callback, void *arg);
    static void _static_callback(void *arg);

    callback_function_t _cb = nullptr;
    esp_timer_handle_t _timer = nullptr;
};

#else // Mac

//**************************************************************************
//                                   Mac
//**************************************************************************

#include <functional>
#include <thread>
#include <atomic>

class Ticker
{
public:
    using callback_t = std::function<void(void)>;
    
    ~Ticker() { stop(); }
    
    void once_ms(uint32_t ms, callback_t callback)
    {
        _ms = ms;
        _cb = callback;
        _run = true;

        std::thread([this]() { 
            std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
            _cb();
        }).detach();
    }
    
    void attach_ms(uint64_t ms, callback_t callback)
    {
        _ms = ms;
        _cb = callback;
        _run = true;

        _thread = std::thread([this]() {
            _id = std::this_thread::get_id();
            while (true) {
                auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(_ms);
                _cb();
                {
                    std::unique_lock<std::mutex> lk(_mutex);
                    if (!_run) {
                        break;
                    }
                }
                std::this_thread::sleep_until(x);
            }
        });
    }

    void stop()
    {
        {
            std::unique_lock<std::mutex> lk(_mutex);
            _run = false;
        }
        
        // Don't try to join ourselves
        if (_id == std::this_thread::get_id()) {
            return;
        }
        
        if (_thread.joinable()) {
            _thread.join();
        }
    }
    
private:
    uint64_t _ms = 0;
    callback_t _cb = nullptr;
    std::thread _thread;
    std::thread::id _id;
    std::mutex _mutex;
    bool _run = false;
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
    
    // Button Simulator for Mac
    static void setButtonDown(bool down);
    
    // This interfaces to LEDs. It can be a single LED connected to a GPIO
    // pin, an addressable RGB LED, or a strip of addressable LEDs.
    static void initLED(uint8_t channel, uint8_t pin, uint32_t numLEDs);
    static void setLED(uint8_t channel, uint32_t index, uint8_t r, uint8_t g, uint8_t b);
    static void refreshLEDs(uint8_t channel);
    
    // Analog
    static void initAnalog(int pin);
    static uint32_t readAnalog(uint8_t pin);
    
    static uint32_t millis();
    
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
        log('V', VerboseColor, tag, fmt, args);
        va_end(args);
    }
    
    static void logD(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('D', DebugColor, tag, fmt, args);
        va_end(args);
    }
    
    static void logI(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('I', InfoColor, tag, fmt, args);
        va_end(args);
    }
    
    static void logW(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('W', WarningColor, tag, fmt, args);
        va_end(args);
    }
    
    static void logE(const char* tag, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log('E', ErrorColor, tag, fmt, args);
        va_end(args);
    }

private:
#ifdef __APPLE__
    static constexpr const char* ErrorColor = "";
    static constexpr const char* WarningColor = "";
    static constexpr const char* InfoColor = "";
    static constexpr const char* VerboseColor = "";
    static constexpr const char* DebugColor = "";
    static constexpr const char* NoColor = "";
#else
    static constexpr const char* ErrorColor = "\x1B[31m";
    static constexpr const char* WarningColor = "\x1B[33m";
    static constexpr const char* InfoColor = "\x1B[32m";
    static constexpr const char* VerboseColor = "\x1B[36m";
    static constexpr const char* DebugColor = "\x1B[35m";
    static constexpr const char* NoColor = "\x1B[0m";
#endif

static void log(char type, const char* color, const char* tag, const char* fmt, va_list args)
    {
        printf("%s%c %s: %s%s\n", color, type, tag, vformat(fmt, args).c_str(), NoColor);
    }
};
