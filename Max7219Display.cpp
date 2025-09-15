/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2017-2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Max7219Display.h"

#include "Font_8x8_8pt.h"
#include "Font_Compact_5pt.h"

using namespace mil;

Max7219Display::Max7219Display(std::function<void()> scrollDone)
	: _matrix(SS, 4, 1)
	, _scrollDone(scrollDone)
{
	pinMode(SS, OUTPUT);
	digitalWrite(SS, LOW);

	_matrix.setTextWrap(false);

	_matrix.setIntensity(0); // Use a value between 0 and 15 for brightness

	_matrix.setPosition(0, 0, 0); // The first display is at <0, 0>
	_matrix.setPosition(1, 1, 0); // The second display is at <1, 0>
	_matrix.setPosition(2, 2, 0); // The third display is at <2, 0>
	_matrix.setPosition(3, 3, 0); // And the last display is at <3, 0>

	_matrix.setRotation(0, 1);    // The first display is position upside down
	_matrix.setRotation(1, 1);    // The first display is position upside down
	_matrix.setRotation(2, 1);    // The first display is position upside down
	_matrix.setRotation(3, 1);    // The same hold for the last display

	clear();
}

uint32_t Max7219Display::getControlChars(const char* s, bool& scroll)
{
	scroll = false;
	
	for (int i = 0; ; ++i) {
        if (s[i] == '\0') {
            break;
        } else if (s[i] == '\a') {
			_matrix.setFont(&Font_Compact_5pt);
			_currentFont = &Font_Compact_5pt;
		} else if (s[i] == '\v') {
			scroll = true;
		} else {
			return i;
		}
	}
    return 0;
}

void Max7219Display::clear()
{
	_matrix.fillScreen(LOW);
	_matrix.write(); // Send bitmap to display
}

void Max7219Display::setBrightness(uint32_t level)
{
	if (level > 15) {
		level = 15;
	}
	_matrix.setIntensity(level);
}
	
void Max7219Display::showString(const char* s, uint32_t underscoreStart, uint32_t underscoreLength)
{
    std::string string(s);
	if (_scrollTimer.active()) {
		_scrollTimer.detach();
	}
	
	_matrix.setFont(&Font_8x8_8pt);
	_currentFont = &Font_8x8_8pt;

	bool scroll;
	uint32_t charStart = getControlChars(string.c_str(), scroll);
	if (scroll) {
		scrollString(string.c_str() + charStart, ScrollRate, ScrollType::Scroll);
		return;
	}

	// center the string
	int16_t x1, y1;
	uint16_t w, h;
	_matrix.getTextBounds((char*) string.c_str() + charStart, 0, 0, &x1, &y1, &w, &h);
	
	if (w > _matrix.width()) {
		// Text is too wide. Do the watusi
		scrollString(string.c_str() + charStart, WatusiRate, ScrollType::WatusiLeft);
		return;
	}


	_matrix.setCursor((_matrix.width() - w) / 2, -y1);
	_matrix.fillScreen(LOW);
	
	// Add underscores if needed
	bool needUnderscore = false;
	if (underscoreLength > 0 && underscoreStart < string.length() - charStart) {
		needUnderscore = true;
		if (underscoreLength + underscoreStart > string.length() - charStart) {
			underscoreLength = string.length() - charStart - underscoreStart;
		}
	}
	
	for (int index = 0; index < string.length() - charStart; ++index) {
		int16_t cursorX = _matrix.getCursorX();
		_matrix.Adafruit_GFX::write(string[index + charStart]);
		if (needUnderscore && index >= underscoreStart && index < underscoreStart + underscoreLength) {
		    _matrix.drawFastHLine(cursorX, _matrix.height() - 1, _matrix.getCursorX() - cursorX, 0xffff);
		}
	}
	
	_matrix.write(); // Send bitmap to display
}

void Max7219Display::scrollString(const char* s, uint32_t scrollRate, ScrollType scrollType)
{
	if (_scrollTimer.active()) {
		_scrollTimer.detach();
	}

	_scrollString = s;

	int16_t x1, y1;
	uint16_t w, h;
	_matrix.getTextBounds((char*) _scrollString.c_str(), 0, 0, &x1, &y1, &w, &h);
	_scrollY = -y1;
	_scrollW = w;
	_scrollType = scrollType;
	
	// If we're doing a full scroll, start offscreen
	_scrollOffset = (scrollType == ScrollType::Scroll) ? _matrix.width() : WatusiMargin;
	_scrollTimer.attach_ms(scrollRate, _scroll, this);
}

static uint8_t getXAdvance(char c, const GFXfont* font)
 {
	 if (!font) {
		 return 0;
	 }
	 
     uint8_t first = pgm_read_byte(&(font->first)),
             last  = pgm_read_byte(&(font->last));
     if((c >= first) && (c <= last)) { // Char present in this font?
         GFXglyph *glyph = font->glyph + (c - first);
         return pgm_read_byte(&glyph->xAdvance);
 	}
 	return 0;
 }

void Max7219Display::scroll()
{
	if (_scrollType == ScrollType::Scroll) {
		if (_scrollOffset-- + _scrollW <= 0) {
			_scrollTimer.detach();
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
	_matrix.fillScreen(LOW);

	for (int i = 0; i < length; ++i) {
		if (x >= _matrix.width()) {
			break;
		}

		uint8_t xAdvance = getXAdvance(_scrollString[i], _currentFont);
		
		if (x + xAdvance > 0) {
			_matrix.setCursor(x, _scrollY);
			_matrix.Adafruit_GFX::write(_scrollString[i]);
		}
		
		x += xAdvance;
	}
	
	_matrix.write(); // Send bitmap to display
}
