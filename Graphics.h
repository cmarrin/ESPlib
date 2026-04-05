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
    
    uint32_t rgbToColor(uint8_t r, uint8_t g, uint8_t b);
    uint32_t hsvToColor(uint16_t h, uint8_t s = 255, uint8_t v = 255);
  
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
