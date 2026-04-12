/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "DSP7S04B.h"

static const char* TAG = "DSP7S04B";

#if defined ESP_PLATFORM
#include "driver/i2c_master.h"

static constexpr uint8_t I2CDeviceAddr = 0x32;    
static constexpr gpio_num_t I2CSDA = gpio_num_t(4);
static constexpr gpio_num_t I2CSCL = gpio_num_t(3);

static i2c_master_bus_handle_t i2c_bus_handle = NULL;   /*!< I2C master bus handle */
static i2c_master_dev_handle_t display_dev_handle = NULL;  /*!< Display device handle */

enum class I2CCmd {
    SetRaw = 0x02,          // 2 bytes per digit, starting from the left (see format above)
    Clear = 0x05,
    SetBrightness = 0x06,   // [level (0-255)
};

#endif

using namespace mil;

// Character map.
//
// This was taken from https://github.com/dmadison/LED-Segment-ASCII

static const uint8_t SevenSegmentASCII[96] = {
	0b00000000, /* (space) */
	0b10000110, /* ! */
	0b00100010, /* " */
	0b01111110, /* # */
	0b01101101, /* $ */
	0b11010010, /* % */
	0b01000110, /* & */
	0b00100000, /* ' */
	0b00101001, /* ( */
	0b00001011, /* ) */
	0b00100001, /* * */
	0b01110000, /* + */
	0b00010000, /* , */
	0b01000000, /* - */
	0b10000000, /* . */
	0b01010010, /* / */
	0b00111111, /* 0 */
	0b00000110, /* 1 */
	0b01011011, /* 2 */
	0b01001111, /* 3 */
	0b01100110, /* 4 */
	0b01101101, /* 5 */
	0b01111101, /* 6 */
	0b00000111, /* 7 */
	0b01111111, /* 8 */
	0b01101111, /* 9 */
	0b00001001, /* : */
	0b00001101, /* ; */
	0b01100001, /* < */
	0b01001000, /* = */
	0b01000011, /* > */
	0b11010011, /* ? */
	0b01011111, /* @ */
	0b01110111, /* A */
	0b01111100, /* B */
	0b00111001, /* C */
	0b01011110, /* D */
	0b01111001, /* E */
	0b01110001, /* F */
	0b00111101, /* G */
	0b01110110, /* H */
	0b00110000, /* I */
	0b00011110, /* J */
	0b01110101, /* K */
	0b00111000, /* L */
	0b00010101, /* M */
	0b00110111, /* N */
	0b00111111, /* O */
	0b01110011, /* P */
	0b01101011, /* Q */
	0b00110011, /* R */
	0b01101101, /* S */
	0b01111000, /* T */
	0b00111110, /* U */
	0b00111110, /* V */
	0b00101010, /* W */
	0b01110110, /* X */
	0b01101110, /* Y */
	0b01011011, /* Z */
	0b00111001, /* [ */
	0b01100100, /* \ */
	0b00001111, /* ] */
	0b00100011, /* ^ */
	0b00001000, /* _ */
	0b00000010, /* ` */
	0b01011111, /* a */
	0b01111100, /* b */
	0b01011000, /* c */
	0b01011110, /* d */
	0b01111011, /* e */
	0b01110001, /* f */
	0b01101111, /* g */
	0b01110100, /* h */
	0b00010000, /* i */
	0b00001100, /* j */
	0b01110101, /* k */
	0b00110000, /* l */
	0b00010100, /* m */
	0b01010100, /* n */
	0b01011100, /* o */
	0b01110011, /* p */
	0b01100111, /* q */
	0b01010000, /* r */
	0b01101101, /* s */
	0b01111000, /* t */
	0b00011100, /* u */
	0b00011100, /* v */
	0b00010100, /* w */
	0b01110110, /* x */
	0b01101110, /* y */
	0b01011011, /* z */
	0b01000110, /* { */
	0b00110000, /* | */
	0b01110000, /* } */
	0b00000001, /* ~ */
	0b00000000, /* (del) */
};

DSP7S04B::DSP7S04B(RenderCB renderCB) :_renderCB(renderCB)
{
    // The canvas has 2 bytes for each digit, starting from the left
    _canvas.begin(8, 1);
    
#if defined ESP_PLATFORM
    i2c_master_bus_config_t bus_config;
    memset(&bus_config, 0, sizeof(bus_config));
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.sda_io_num = I2CSDA;
    bus_config.scl_io_num = I2CSCL;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.intr_priority = 0;
    bus_config.trans_queue_depth = 0;  /* 0 = synchronous mode */
    bus_config.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2CDeviceAddr,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &display_dev_handle));

#endif
}

void
DSP7S04B::print(const char* str)
{
    for (uint8_t count = 0; count < 4; count++) {
        uint8_t c = uint8_t(str[count]);
        if (c == '\0') {
            break;
        }
        if (c < 0x20 || c > 0x7f) {
            continue;
        }
        
        uint8_t* b = reinterpret_cast<uint8_t*>(_canvas.getBuffer());
        b[count * 2] = SevenSegmentASCII[c - 0x20];
        b[count * 2 + 1] = 0;
    }
}

void
DSP7S04B::setDot(uint8_t pos, bool on)
{
    if (pos > 3) {
        return;
    }
    uint8_t* b = reinterpret_cast<uint8_t*>(_canvas.getBuffer());
    if (on) {
        b[pos * 2] |= 0x80;
    } else {
        b[pos * 2] &= 0x7f;
    }
}

void
DSP7S04B::setColon(bool on)
{
    // Colon is the lsb of the rightmost digit
    uint8_t* b = reinterpret_cast<uint8_t*>(_canvas.getBuffer());
    if (on) {
        b[7] |= 0x01;
    } else {
        b[7] &= 0xfe;
    }
}

void
DSP7S04B::clearDisplay()
{
    _canvas.fillScreen(0);
}	

#if defined ESP_PLATFORM

void
DSP7S04B::setBrightness(uint8_t level)
{
    uint8_t buf[2] = { uint8_t(I2CCmd::SetBrightness), level };
    esp_err_t result = i2c_master_transmit(display_dev_handle, buf, sizeof(buf), -1);
    if (result != 0) {
        System::logE(TAG, "Error setting display brightness (%d)", int(result));
    }
}

void DSP7S04B::refresh()
{
    uint8_t buf[9] = { uint8_t(I2CCmd::SetRaw) };
    uint8_t* b = reinterpret_cast<uint8_t*>(_canvas.getBuffer());
    memcpy(buf + 1, b, 8);
    buf[1] = 0xff;
    esp_err_t result = i2c_master_transmit(display_dev_handle, buf, sizeof(buf), -1);
    if (result != 0) {
        System::logE(TAG, "Error sending raw data to display (%d)", int(result));
    }
}

#else

void DSP7S04B::setBrightness(uint8_t level)
{
}

void DSP7S04B::refresh()
{
    if (_renderCB) {
        _renderCB(&_canvas);
    }
}

#endif
