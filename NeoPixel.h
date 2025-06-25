/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// Wrapper around Adafruit NeoPixel driver

#pragma once

#include "mil.h"

#ifdef ARDUINO
#include <Adafruit_NeoPixel.h>

namespace mil {

class NeoPixel
{
public:
    NeoPixel(uint16_t numPixels, uint8_t ledPin) : _pixels(numPixels, ledPin, NEO_GRB + NEO_KHZ800) { }

    void begin() { _pixels.begin(); }
    void setBrightness(uint8_t b) { _pixels.setBrightness(b); }
    uint16_t numPixels() const { return _pixels.numPixels(); }
    voif show() { _pixels.show(); }
    void setPixelColor(uint16_t i, uint32_t color)
    {
        _pixels.setPixelColor(i, color(h,s,v));
    }
    
    uint32_t color(uint8_t h, uint8_t s, uint8_t v) { return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(uint16_t(h) * 256, s, v)); }

private:
    Adafruit_NeoPixel _pixels
};

}

#else

namespace mil {

class NeoPixel
{
public:
    NeoPixel(uint16_t numPixels, uint8_t ledPin) : _numPixels(numPixels) { }

    void begin() { }
    void setBrightness(uint8_t b) { }
    uint16_t numPixels() const { return _numPixels; }
    void show() { }
    void setPixelColor(uint16_t i, uint32_t color)
    {
        char buf[3] = "00";
        cout << ("setLight(") << i << ", 0x";
        mil::toHex(color >> 24, buf);
        cout << buf;
        mil::toHex(color >> 16, buf);
        cout << buf;
        mil::toHex(color >> 8, buf);
        cout << buf;
        mil::toHex(color, buf);
        cout << buf;
        cout << "\n";
    }

    uint32_t color(uint8_t h, uint8_t s, uint8_t v) { return (uint32_t(h) << 16) | (uint32_t(s) << 8) | uint32_t(v); }

  private:
    uint16_t _numPixels = 0;
};

}
#endif
