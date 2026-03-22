/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2017-2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Max7219Display.h"

// Ignore PROGMEM in fonts
#define PROGMEM
#include "Font_8x8_8pt.h"
#include "Font_Compact_5pt.h"

#if defined ESP_PLATFORM
#include <max7219.h>

// These values are for the ESP32C3
static constexpr int CASCADE_SIZE = 4;
static constexpr spi_host_device_t HOST = SPI2_HOST;
static constexpr int MOSI = 4;
static constexpr int CLK = 2;
static constexpr int CS = 7;

static max7219_t dev =
{
    .digits = 0,
    .cascade_size = CASCADE_SIZE,
    .mirrored = true
};

#endif

using namespace mil;

Max7219Display::Max7219Display(std::function<void()> scrollDone, std::function<void(const uint8_t* buffer)> renderCB)
	: _matrix(32, 8)
    , _scrollDone(scrollDone)
    , _renderCB(renderCB)
{
#if defined ESP_PLATFORM
    // Configure SPI bus
    spi_bus_config_t cfg =
    {
        .mosi_io_num = MOSI,
        .miso_io_num = -1,
        .sclk_io_num = CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, SPI_DMA_CH_AUTO));

    // Configure device
    ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, (gpio_num_t) CS));
    ESP_ERROR_CHECK(max7219_init(&dev));
    
    _matrix.setFlip(true, false);
 #endif
	clear();
}

void Max7219Display::clear()
{
    _matrix.fillScreen(0);
    refresh();
}

void Max7219Display::setBrightness(uint32_t level)
{
	if (level > 15) {
		level = 15;
	}
#if defined ESP_PLATFORM
    max7219_set_brightness(&dev, level);
#endif
}
	
void Max7219Display::showString(const char* s)
{
    std::string string(s);
    _scrollTimer.stop();
	
	_matrix.setFont(&Font_8x8_8pt);

	bool scroll;
	uint32_t charStart = getControlChars(string.c_str(), scroll);
	if (scroll) {
		scrollString(string.c_str() + charStart, ScrollRate, ScrollType::Scroll);
		return;
	}

	// center the string
	int16_t x1, y1;
	uint16_t w, h;
	_matrix.getTextBounds((char*) string.c_str() + charStart, 0, 0, x1, y1, w, h);
	
	if (w > _matrix.width()) {
		// Text is too wide. Do the watusi
		scrollString(string.c_str() + charStart, WatusiRate, ScrollType::WatusiLeft);
		return;
	}

	_matrix.setCursor((_matrix.width() - w) / 2, -y1);
	_matrix.fillScreen(0);
	
	for (int index = 0; index < string.length() - charStart; ++index) {
		_matrix.writeChar(string[index + charStart]);
	}
	
    refresh(); // Send bitmap to display
}

void Max7219Display::scrollString(const char* s, uint32_t scrollRate, ScrollType scrollType)
{
    _scrollTimer.stop();

	_scrollString = s;

	int16_t x1, y1;
	uint16_t w, h;
	_matrix.getTextBounds((char*) _scrollString.c_str(), 0, 0, x1, y1, w, h);
	_scrollY = -y1;
	_scrollW = w;
	_scrollType = scrollType;
	
	// If we're doing a full scroll, start offscreen
	_scrollOffset = (scrollType == ScrollType::Scroll) ? _matrix.width() : WatusiMargin;
	_scrollTimer.attach_ms(scrollRate, [this]() { scroll(); });
}

uint32_t Max7219Display::getControlChars(const char* s, bool& scroll)
{
	scroll = false;
	
	for (int i = 0; ; ++i) {
        if (s[i] == '\0') {
            break;
        } else if (s[i] == '\a') {
			_matrix.setFont(&Font_Compact_5pt);
		} else if (s[i] == '\v') {
			scroll = true;
		} else {
			return i;
		}
	}
    return 0;
}

void Max7219Display::scroll()
{
	if (_scrollType == ScrollType::Scroll) {
		if (_scrollOffset-- + _scrollW <= 0) {
			_scrollTimer.stop();
			_scrollDone();
			return;
		}
	} else if (_scrollType == ScrollType::WatusiLeft) {
		if (_scrollOffset-- <= _matrix.width() - _scrollW - WatusiMargin) {
			_scrollType = ScrollType::WatusiRight;
		}
	} else if (_scrollOffset++ >= WatusiMargin) {
			_scrollType = ScrollType::WatusiLeft;
	}
	
	size_t length = _scrollString.length();
	int16_t x = _scrollOffset;
	_matrix.fillScreen(0);

	for (int i = 0; i < length; ++i) {
		if (x >= _matrix.width()) {
			break;
		}

		uint8_t xAdvance = _matrix.getXAdvance(_scrollString[i]);
		
		if (x + xAdvance > 0) {
			_matrix.setCursor(x, _scrollY);
			_matrix.writeChar(_scrollString[i]);
		}
		
		x += xAdvance;
	}
	
	refresh(); // Send bitmap to display
}

void
Max7219Display::refresh()
{
#if defined ESP_PLATFORM
    // Write to the display
    uint8_t* buffer = _matrix.getBuffer();
    
    for (int i = 0; i < 32; ++i) {
        uint32_t row = (3 - (i % 4)) * 8 + (i / 4);
        max7219_set_digit(&dev, row, buffer[i]);
    }
#else
    if (_renderCB) {
        _renderCB(_matrix.getBuffer());
    }
#endif
}
