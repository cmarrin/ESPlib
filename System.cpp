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
    System::gpioSetPinMode(LED_BUILTIN, System::GPIOPinMode::Output);
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

static led_strip_handle_t ledStrip[NumLEDChannels];
static bool ledStripInited[NumLEDChannels] = { 0 };

void
System::initLED(uint8_t channel, uint8_t pin, uint32_t numLeds)
{
    if (HaveAddressableRGBLED) {
        if (channel >= NumLEDChannels) {
            return;
        }
        
        if (ledStripInited[channel]) {
            led_strip_del(ledStrip[channel]);
            ledStripInited[channel] = false;
        }

        led_strip_config_t strip_config = {
            .strip_gpio_num = pin,
            .max_leds = numLeds,
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
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &ledStrip[channel]));
        ledStripInited[channel] = true;
    } else {
        gpioSetPinMode(LED_BUILTIN, GPIOPinMode::Output);
    }
}

void
System::setLED(uint8_t channel, uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (HaveAddressableRGBLED) {
        if (channel >= NumLEDChannels) {
            return;
        }
        
        ESP_ERROR_CHECK(led_strip_set_pixel(ledStrip[channel], index, r, g, b));
    } else {
        bool on = r == 0 && g == 0 && b == 0;
        gpioWritePin(LED_BUILTIN, on);
    }
}

void
System::refreshLEDs(uint8_t channel)
{
    if (HaveAddressableRGBLED) {
        if (channel >= NumLEDChannels) {
            return;
        }
        
        ESP_ERROR_CHECK(led_strip_refresh(ledStrip[channel]));
    }
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

static constexpr adc_channel_t ADCChannel = ADC_CHANNEL_1;
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
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, ADCChannel, &config));
}

uint32_t
System::readAnalog(uint8_t pin)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, ADCChannel, &raw));
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

std::function<void(const void* buffer)> _renderCB;
GraphicsCanvas24 _canvas;

void
System::setRenderCB(std::function<void(const void* buffer)> renderCB)
{
    _renderCB = renderCB;
}

void
System::initLED(uint8_t channel, uint8_t pin, uint32_t numLeds)
{
    if (channel == 1) {
        // This is the GraphicsCanvas24 used for neopixels
        if (_canvas.getBuffer()) {
            // Already initialized
            return;
        }
        _canvas.begin(numLeds, 1);
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
        _renderCB(_canvas.getBuffer());
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

uint32_t
System::millis()
{
    return uint32_t(double(clock()) / CLOCKS_PER_SEC * 1000);
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
