/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2026, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Graphics.h"

#include <cstring>

using namespace mil;

Graphics::Graphics(int16_t w, int16_t h)
    : WIDTH(w)
    , HEIGHT(h)
{
    _width = WIDTH;
    _height = HEIGHT;
    _rotation = Rotation::Angle0;
    _cursorY = _cursorX = 0;
    _font = nullptr;
}

// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

void
Graphics::writeChar(uint8_t c)
{
    if (!_font) {
        return;
    }
    
    if (c == '\n') {
        _cursorX = 0;
        _cursorY += _font->yAdvance;
    } else if (c != '\r') {
        uint8_t first = _font->first;
        
        if (c >= first && c <= _font->last) {
            GFXglyph* glyph = _font->glyph + (c - first);
            if (glyph->width > 0 && glyph->height > 0) {
                drawChar(_cursorX, _cursorY, c, 1, 0);
            }
            _cursorX += glyph->xAdvance;
        }
    }
}

void
Graphics::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg)
{
    c -= _font->first;
    GFXglyph* glyph = _font->glyph + c;
    uint8_t* bitmap = _font->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->xOffset;
    int8_t yo = glyph->yOffset;
    uint8_t bits = 0;
    uint8_t bit = 0;

    for (uint8_t yy = 0; yy < h; yy++) {
        for (uint8_t xx = 0; xx < w; xx++) {
            if (!(bit++ & 7)) {
                bits = bitmap[bo++];
            }
            
            if (bits & 0x80) {
                drawPixel(x + xo + xx, y + yo + yy, color);
            }
            bits <<= 1;
        }
    }
}

void
Graphics::setRotation(Rotation x)
{
    switch (_rotation) {
        case Rotation::Angle0:
        case Rotation::Angle180:
            _width = WIDTH;
            _height = HEIGHT;
            break;
        case Rotation::Angle90:
        case Rotation::Angle270:
            _width = HEIGHT;
            _height = WIDTH;
            break;
    }
}

void
Graphics::charBounds(char c, int16_t& x, int16_t& y, int16_t& minx, int16_t& miny, int16_t& maxx, int16_t& maxy)
{
    if (!_font) {
        x = y = minx = miny = maxx = maxy = 0;
        return;
    }

    if (c == '\n') {
        // Reset x to zero, advance y by one line
        x = 0;
        y += _font->yAdvance;
    } else if (c != '\r') {
        // Not a carriage return; is normal char
        uint8_t first = _font->first;
        uint8_t last = _font->last;
        
        if (c >= first && c <= last) {
            GFXglyph* glyph = _font->glyph + (c - first);
            uint8_t gw = glyph->width;
            uint8_t gh = glyph->height;
            uint8_t xa = glyph->xAdvance;
            int8_t  xo = glyph->xOffset;
            int8_t  yo = glyph->yOffset;
            
            int16_t x1 = x + xo;
            int16_t y1 = y + yo;
            int16_t x2 = x1 + gw - 1;
            int16_t y2 = y1 + gh - 1;
            
            if (x1 < minx) {
                minx = x1;
            }
            if (y1 < miny) {
                miny = y1;
            }
            if (x2 > maxx) {
                maxx = x2;
            }
            if (y2 > maxy) {
                maxy = y2;
            }
            x += xa;
        }
    }
}

void
Graphics::getTextBounds(const char *str, int16_t x, int16_t y, int16_t& x1, int16_t& y1, uint16_t& w, uint16_t& h)
{
    uint8_t c; // Current character

    x1 = x;
    y1 = y;
    w = h = 0;

    int16_t minx = _width;
    int16_t miny = _height;
    int16_t maxx = -1;
    int16_t maxy = -1;

    while ((c = *str++)) {
        charBounds(c, x, y, minx, miny, maxx, maxy);
    }

    if (maxx >= minx) {
        x1 = minx;
        w = maxx - minx + 1;
    }
    if (maxy >= miny) {
        y1 = miny;
        h = maxy - miny + 1;
    }
}

GraphicsCanvas1::GraphicsCanvas1(uint16_t w, uint16_t h)
    : Graphics(w, h)
{
    uint16_t bytes = ((w + 7) / 8) * h;
    _buffer = new uint8_t[bytes];
    if (_buffer != nullptr) {
        memset(_buffer, 0, bytes);
    }
}

GraphicsCanvas1::~GraphicsCanvas1()
{
    delete [ ] _buffer;
}

void
GraphicsCanvas1::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (!_buffer || x < 0 || y < 0 || x >= _width || y >= _height) {
        return;
    }
    
    if (_flipX) {
        x = _width - x - 1;
    }
    if (_flipY) {
        y = _height - y - 1;
    }

    int16_t t;
    switch (_rotation) {
        case Rotation::Angle0: break;
        case Rotation::Angle90:
            t = x;
            x = WIDTH - 1 - y;
            y = t;
            break;
        case Rotation::Angle180:
            x = WIDTH - 1 - x;
            y = HEIGHT - 1 - y;
            break;
        case Rotation::Angle270:
            t = x;
            x = y;
            y = HEIGHT - 1 - t;
            break;
    }

    uint8_t* ptr = &_buffer[(x / 8) + y * ((WIDTH + 7) / 8)];
    if (color) {
        *ptr |= 0x80 >> (x & 7);
    } else {
      *ptr &= ~(0x80 >> (x & 7));
    }
}

void
GraphicsCanvas1::fillScreen(uint16_t color)
{
    if (_buffer) {
        uint16_t bytes = ((WIDTH + 7) / 8) * HEIGHT;
        memset(_buffer, color ? 0xff : 0x00, bytes);
    }
}
