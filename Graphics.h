/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2026, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// This is a heavily modified version of Adafruit_GFX library:
//
//      https://github.com/adafruit/Adafruit-GFX-Library/
//
// It builds with the espidf tools in VSCode. It is simplified to only include 
// the GFXfont and String rendering support. It now longer subclasses the 
// Arduino Print class so you have to use the specific string drawing 
// functionality in this class. It uses the gfxfont format which gives
// it access to fonts created with the Adafruit GFX Pixel Font Customizer:
//
//      https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
//
// as well as other font conversion tools.

#pragma once

#include "mil.h"

// From gfxfont.h

// Font data stored PER GLYPH
struct GFXglyph {
    uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
    uint8_t width;         ///< Bitmap dimensions in pixels
    uint8_t height;        ///< Bitmap dimensions in pixels
    uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
    int8_t xOffset;        ///< X dist from cursor pos to UL corner
    int8_t yOffset;        ///< Y dist from cursor pos to UL corner
};

// Data stored for FONT AS A WHOLE
struct GFXfont {
    uint8_t* bitmap;  ///< Glyph bitmaps, concatenated
    GFXglyph* glyph;  ///< Glyph array
    uint16_t first;   ///< ASCII extents (first char)
    uint16_t last;    ///< ASCII extents (last char)
    uint8_t yAdvance; ///< Newline distance (y axis)
};

namespace mil {

class Graphics {
  public:
    virtual void begin(uint16_t w, uint16_t h);
    virtual void end() { }

    virtual void drawPixel(int16_t x, int16_t y, uint32_t color) = 0;
    virtual void* getBuffer() const { return nullptr; }
    
    static uint32_t rgbToColor(uint8_t r, uint8_t g, uint8_t b);
    static void hsvToRGB(uint8_t& r, uint8_t& g, uint8_t& b, uint16_t h, uint8_t s = 255, uint8_t v = 255);
    static uint8_t gamma(uint8_t c) { return _gammaTable[c]; }
    void setFont(const GFXfont* f = nullptr) { _font = f; }
    void writeChar(uint8_t c);
    
    enum class Rotation { Angle0, Angle90, Angle180, Angle270 };
    
    void setRotation(Rotation r);
    void setFlip(bool flipX, bool flipY) { _flipX = flipX; _flipY = flipY; }
    void setCursor(int16_t x, int16_t y) { _cursorX = x; _cursorY = y; }
    int16_t getCursorX(void) const { return _cursorX; }
    int16_t getCursorY(void) const { return _cursorY; }
    int16_t width() const { return _width; };
    int16_t height() const { return _height; };
    uint8_t getXAdvance(char c) const
    {
        if (_font && c >= _font->first && c <= _font->last) {
            return _font->glyph[c - _font->first].xAdvance;
        }
        return 0;
    }

    void getTextBounds(const char* string, int16_t x, int16_t y, int16_t& x1, int16_t& y1, uint16_t& w, uint16_t& h);
    
  protected:
    void drawChar(int16_t x, int16_t y, unsigned char c, uint32_t color, uint32_t bg);
    void charBounds(char c, int16_t& x, int16_t& y, int16_t& minx, int16_t& miny, int16_t& maxx, int16_t& maxy);

    int16_t WIDTH = 0;      ///< This is the 'raw' display width - never changes
    int16_t HEIGHT = 0;     ///< This is the 'raw' display height - never changes
    int16_t _width = 0;     ///< Display width as modified by current rotation
    int16_t _height = 0;    ///< Display height as modified by current rotation
    int16_t _cursorX = 0;   ///< x location to start print()ing text
    int16_t _cursorY = 0;   ///< y location to start print()ing text
    Rotation _rotation = Rotation::Angle0;
    bool _flipX = false;
    bool _flipY = false;

    const GFXfont* _font = nullptr;

  private:
    static constexpr uint8_t _gammaTable[256] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,
        3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,
        6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,
        11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,
        17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
        25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,
        36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  48,
        49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
        64,  65,  66,  68,  69,  70,  71,  72,  73,  75,  76,  77,  78,  80,  81,
        82,  84,  85,  86,  88,  89,  90,  92,  93,  94,  96,  97,  99,  100, 102,
        103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 119, 120, 122, 124, 125,
        127, 129, 130, 132, 134, 136, 137, 139, 141, 143, 145, 146, 148, 150, 152,
        154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182,
        184, 186, 188, 191, 193, 195, 197, 199, 202, 204, 206, 209, 211, 213, 215,
        218, 220, 223, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250, 252,
        255
    };
};

// A GFX 1-bit canvas context for graphics
class GraphicsCanvas1 : public Graphics
{
  public:
    virtual void begin(uint16_t w, uint16_t h) override;
    virtual void end() override;
    
    virtual void drawPixel(int16_t x, int16_t y, uint32_t color) override;
    void fillScreen(uint32_t color);
    virtual void* getBuffer() const override{ return _buffer; }

  private:
    uint8_t* _buffer;
};

// A GFX 24 bit RGB canvas context for graphics
class GraphicsCanvas24 : public Graphics
{
  public:
    virtual void begin(uint16_t w, uint16_t h) override;
    virtual void end() override;
    
    virtual void drawPixel(int16_t x, int16_t y, uint32_t color) override;
    void fillScreen(uint32_t color);
    virtual void* getBuffer() const override{ return _buffer; }

  private:
    uint32_t* _buffer;
};

}
