/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2017-2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "Graphics.h"
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
    Max7219Display(std::function<void()> scrollDone, std::function<void(const uint8_t* buffer)> renderCB);

    void clear();
    void setBrightness(uint32_t level);
    void showString(const char* str);

    void refresh();

private:
    static constexpr uint32_t ScrollRate = 50;
    static constexpr uint32_t WatusiRate = 80;
    static constexpr int32_t WatusiMargin = 3;
    
    enum class ScrollType { Scroll, WatusiLeft, WatusiRight };

    void scrollString(const char* s, uint32_t scrollRate, ScrollType);
    void scroll();

    // Look for control chars. \a means to use the compact font, \v means to scroll.
    // Returns offset into string past control chars
    uint32_t getControlChars(const char* s, bool& scroll);
    
    GraphicsCanvas1 _matrix;
    Ticker _scrollTimer;
    std::string _scrollString;
    int32_t _scrollOffset;
    int16_t _scrollY;
    int32_t _scrollW;
    ScrollType _scrollType;
    std::function<void()> _scrollDone;
    std::function<void(const uint8_t* buffer)> _renderCB;
};

}
