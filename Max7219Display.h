/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2017-2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include <mil.h>

#include "System.h"

// Control Max7219 8x8 matrix displays. For how we assume 4 displays in a
// horizontal row. We will create a bitmap that is 32x8x1 pixels. We will
// write characters into this display and then send it to the hardware.
// We'll adapt the AdafruitGFX library to manage the font used to write
// characters.

namespace mil {

class Max7219Display
{
public:
    Max7219Display(std::function<void()> scrollDone);

    void clear();
    void setBrightness(uint32_t level);
    void showString(const char* str, uint32_t underscoreStart = 0, uint32_t underscoreLength = 0);

private:
    static constexpr uint32_t ScrollRate = 50;
    static constexpr uint32_t WatusiRate = 80;
    static constexpr int32_t WatusiMargin = 3;
    
    enum class ScrollType { Scroll, WatusiLeft, WatusiRight };

    void scrollString(const char* s, uint32_t scrollRate, ScrollType);
    void scroll();
    static void _scroll(Max7219Display* self) { self->scroll(); }

    // Look for control chars. \a means to use the compact font, \v means to scroll.
    // Returns offset into string past control chars
    uint32_t getControlChars(const char* s, bool& scroll);
    
    void writePixel(uint32_t x, uint32_t y, bool on);

    const GFXfont* _currentFont = nullptr;
    Ticker _scrollTimer;
    std::string _scrollString;
    int32_t _scrollOffset;
    int16_t _scrollY;
    int32_t _scrollW;
    ScrollType _scrollType;
    std::function<void()> _scrollDone;
    
    // Frame buffer is 32 x 8 x 1 pixels
    uint8_t _frameBuffer[32];
};

}
