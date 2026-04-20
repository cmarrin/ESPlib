/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "System.h"

using namespace mil;

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
System::initLED(uint8_t channel, uint8_t pin, uint32_t numLEDs)
{
    System::gpioSetPinMode(pin, System::GPIOPinMode::Output);
}

void
System::setLED(uint8_t channel, uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    bool on = r == 0 && g == 0 && b == 0;
    System::gpioWritePin(LED_BUILTIN, on);
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
    state = state ^ ActiveHighLED;
    digitalWrite(pin, state ? HIGH : LOW);
}

bool
System::gpioReadPin(uint8_t pin)
{
    return digitalRead(pin) != 0;
}

void
System::setButtonDown(bool down)
{
}

void 
System::initAnalog(int pin)
{
}

uint32_t
System::readAnalog(uint8_t pin)
{
	return analogRead(pin);
}

uint32_t
System::millis()
{
    return ::millis();
}

void
System::restart()
{
    ESP.restart();
}

bool
System::isRestarting()
{
    return false;
}

#elif defined ESP_PLATFORM

//**************************************************************************
//                                   esp-idf
//**************************************************************************

#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_adc/adc_oneshot.h"

static constexpr int NumLEDChannels = 4;

struct LEDConfig
{
    led_strip_handle_t handle;
    bool isInited;
    bool isAddressable;
    uint8_t pin;
    uint32_t numLEDs;
};

static LEDConfig ledStrip[NumLEDChannels] = { 0 };

// Channel 0 is used for the Blinker. It can be addressable
// or not. The other are always addressable

void
System::initLED(uint8_t channel, uint8_t pin, uint32_t numLEDs)
{
    if (channel >= NumLEDChannels) {
        return;
    }
    
    if (channel != 0 || HaveAddressableRGBLED) {
        
        if (ledStrip[channel].isInited) {
            led_strip_del(ledStrip[channel].handle);
            ledStrip[channel].isInited = false;
        }

        led_strip_config_t strip_config = {
            .strip_gpio_num = pin,
            .max_leds = numLEDs,
            .led_model = LED_MODEL_WS2812,
            .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
            .flags = { .invert_out = false },
        };

        // LED strip backend configuration: RMT
        // FIXME: use AddressableLEDRMT to decide on RMT or SPI
        led_strip_rmt_config_t rmt_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
            .resolution_hz = 10 * 1000 * 1000,
            .mem_block_symbols = 0,
            .flags = {
                .with_dma = false,
            }
        };

        // LED Strip object handle
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(ledStrip[channel].handle)));
        ledStrip[channel].isAddressable = true;
    } else {
        gpioSetPinMode(pin, GPIOPinMode::Output);
        ledStrip[channel].isAddressable = false;
    }
    ledStrip[channel].pin = pin;
    ledStrip[channel].isInited = true;
    ledStrip[channel].numLEDs = numLEDs;
}

void
System::setLED(uint8_t channel, uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (channel >= NumLEDChannels || !ledStrip[channel].isInited) {
        return;
    }
    
    if (ledStrip[channel].isAddressable && index < ledStrip[channel].numLEDs) {
        r = Graphics::gamma(r);
        g = Graphics::gamma(g);
        b = Graphics::gamma(b);
        ESP_ERROR_CHECK(led_strip_set_pixel(ledStrip[channel].handle, index, r, g, b));
    } else {
        bool off = r == 0 && g == 0 && b == 0;
        gpioWritePin(ledStrip[channel].pin, !off);
    }
}

void
System::refreshLEDs(uint8_t channel)
{
    if (channel >= NumLEDChannels || !ledStrip[channel].isInited || !ledStrip[channel].isAddressable) {
        return;
    }
        
    ESP_ERROR_CHECK(led_strip_refresh(ledStrip[channel].handle));
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
    state = state ^ ActiveHighLED;
    gpio_set_level(gpio_num_t(pin), state);
}

bool
System::gpioReadPin(uint8_t pin)
{
    return gpio_get_level(gpio_num_t(pin)) != 0;
}

void
System::setButtonDown(bool down)
{
}

static adc_oneshot_unit_handle_t adcHandle;

void 
System::initAnalog(int pin)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adcHandle));
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adc_channel_t(pin), &config));
}

uint32_t
System::readAnalog(uint8_t pin)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adc_channel_t(pin), &raw));
    if (raw < 0) {
        raw = 0;
    }
    
    return uint32_t(raw);
}

uint32_t
System::millis()
{
    return uint32_t(esp_timer_get_time() / 1000);
}

void
System::restart()
{
    esp_restart();
}    

bool
System::isRestarting()
{
    return false;
}

Ticker::~Ticker()
{
    stop();
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
Ticker::stop()
{
    if (_timer) {
        esp_timer_stop(_timer);
        esp_timer_delete(_timer);
        _timer = nullptr;
        _cb = nullptr;
    }
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

RenderCB _renderCB;
GraphicsCanvas24 _canvas;

void
System::setRenderCB(RenderCB renderCB)
{
    _renderCB = renderCB;
}

void
System::initLED(uint8_t channel, uint8_t pin, uint32_t numLEDs)
{
    if (channel == 1) {
        // This is the GraphicsCanvas24 used for neopixels
        _canvas.begin(numLEDs, 1);
    }
}

void
System::setLED(uint8_t channel, uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (channel == 1 && _canvas.getBuffer() && index < _canvas.width()) {
        _canvas.drawPixel(index, 0, _canvas.rgbToColor(r, g, b));
    }
}

void
System::refreshLEDs(uint8_t channel)
{
    if (_renderCB && channel == 1) {
        _renderCB(&_canvas);
    }
}

// Simulate a button
static uint8_t _buttonPin = 0;
static bool _buttonIsDown = false;

void
System::gpioSetPinMode(uint8_t pin, GPIOPinMode mode)
{
    _buttonPin = pin;
}

void
System::gpioWritePin(uint8_t pin, bool state)
{
}

bool
System::gpioReadPin(uint8_t pin)
{
    bool v = (pin == _buttonPin) ? _buttonIsDown : false;
    return v;
}

void
System::setButtonDown(bool down)
{
    _buttonIsDown = down;
}

void 
System::initAnalog(int pin)
{
}

uint32_t
System::readAnalog(uint8_t pin)
{
	return 0;
}

std::chrono::steady_clock::time_point _startTime;

uint32_t
System::millis()
{
    if (_startTime == std::chrono::steady_clock::time_point()) {
        _startTime = std::chrono::steady_clock::now();
    }
    return ((std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - _startTime)).count()) * 1000;
}

static bool _restarting = false;

void
System::restart()
{
    _restarting = true;
}    

bool
System::isRestarting()
{
    bool b = _restarting;
    _restarting = false;
    return b;
}

#endif
