/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "System.h"

// Logging
std::string
System::vformat(const char* fmt, va_list args)
{
    int size = vsnprintf(nullptr, 0, fmt, args);
    char* buf = new char[size + 1];
    vsnprintf(buf, size + 1, fmt, args);
    std::string s(buf);
    delete [ ] buf;
    return s;
}

#if defined ARDUINO

//**************************************************************************
//                                   Arduino
//**************************************************************************

void
System::initLED()
{
    System::gpioSetPinMode(LED_BUILTIN, System::GPIOPinMode::Output);
}

void
System::setLED(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    bool b = r == 0 && g == 0 && b == 0;
    System::gpioWritePin(LED_BUILTIN, b);
}

void
System::gpioSetPinMode(uint8_t pin, GPIOPinMode mode)
{
    switch(mode) {
        case GPIOPinMode::Output: pinMode(pin, OUTPUT); return;
        case GPIOPinMode::Input: pinMode(pin, INPUT); return;
        case GPIOPinMode::InputWithPullup: pinMode(pin, INPUT_PULLUP); return;
    }
}

void
System::gpioWritePin(uint8_t pin, bool state)
{
    digitalWrite(pin, state ? HIGH : LOW);
}

bool
System::gpioReadPin(uint8_t pin)
{
    return digitalRead(pin) != 0;
}

uint32_t
System::gpioReadAnalog(uint8_t pin)
{
	return analogRead(pin);
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

#include "driver/gpio.h"

static const char* TAG = "System";

// Assume this is an addressable RGB LED on GPIO pin 8
static constexpr int BLINK_GPIO = 8;

void
System::initLED()
{
}

void
System::setLED(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
}

void
System::gpioSetPinMode(uint8_t pin, GPIOPinMode mode)
{
    gpio_reset_pin(gpio_num_t(pin));
    gpio_set_direction(gpio_num_t(pin), (mode == GPIOPinMode::Output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
}

void
System::gpioWritePin(uint8_t pin, bool state)
{
    gpio_set_level(gpio_num_t(pin), state);
}

bool
System::gpioReadPin(uint8_t pin)
{
    return gpio_get_level(gpio_num_t(pin)) != 0;
}

uint32_t
System::gpioReadAnalog(uint8_t pin)
{
    // FIXME: Implement
	return 0;
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
System::initLED()
{
}

void
System::setLED(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
}

void
System::gpioSetPinMode(uint8_t pin, GPIOPinMode mode)
{
}

void
System::gpioWritePin(uint8_t pin, bool state)
{
}

bool
System::gpioReadPin(uint8_t pin)
{
    return 0;
}

uint32_t
System::gpioReadAnalog(uint8_t pin)
{
	return 0;
}

void
System::restart()
{
    printf("***** RESTART *****\n");
}    

#endif
