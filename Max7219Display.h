/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2017-2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include <mil.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <time.h>

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

    Max72xxPanel _matrix;
    const GFXfont* _currentFont = nullptr;
    Ticker _scrollTimer;
    String _scrollString;
    int32_t _scrollOffset;
    int16_t _scrollY;
    int32_t _scrollW;
    ScrollType _scrollType;
    std::function<void()> _scrollDone;
};

}
