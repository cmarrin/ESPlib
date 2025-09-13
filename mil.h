//
//  mil.h
//
//  Created by Chris Marrin on 3/19/2011.
//
//

/*
Copyright (c) 2009-2011 Chris Marrin (chris@marrin.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, this 
      list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice, 
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution.

    - Neither the name of Marrinator nor the names of its contributors may be 
      used to endorse or promote products derived from this software without 
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE.
*/

#pragma once

/*  This is the minimum file to use AVR
    This file should be included by all files that will use 
    or implement the interfaces
*/

#if defined ARDUINO
#include <Arduino.h>
#include <Printable.h>

#define ToString(v) String(v)
#define ToFloat(s) s.toFloat()

#else
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <functional>

#define cout std::cout
#define F(s) s
#define ToString(v) String(std::to_string(v).c_str())
#define ToFloat(s) std::stof(s)
#define PROGMEM

static inline uint32_t millis() { return uint32_t((double) clock() / CLOCKS_PER_SEC * 1000); }

class String : public std::string
{
  public:
    String() : std::string() { }
    String(const char* s) : std::string(s) { }
    String(std::string& s) : std::string(s) { }
    
    String operator + (String& a) { return *this + a.c_str(); }    
    String operator + (const char* a)
    {
        String result = *this;
        result += a;
        return result;
    }

    bool startsWith(String s) const { return starts_with(s); }
    int16_t indexOf(char val, uint16_t from = 0) const
    {
        int16_t r = find(val, from);
        return (r == npos) ? -1 : r;
    }
    
    String substring(int32_t from) const { return String(substr(from).c_str()); }
    String substring(int32_t from, int32_t to) const { return String(substr(from, size() - to + 1).c_str()); } 
    
    String toString() { return *this; }
};

static inline void delay(uint32_t ms) { useconds_t us = useconds_t(ms) * 1000; usleep(us); }

static constexpr uint8_t LED_BUILTIN = 0;

#define NEO_GRB 0
#define NEO_KHZ800 0

namespace mil {

class DSP7S04B
{
public:
    void setDot(uint8_t pos, bool on) { if (on) { printf("Dot set\n"); } }
    void setColon(bool on) { _colon = on; }
    void clearDisplay(void) { }
    void setBrightness(uint8_t b) { printf("*** Brightness set to %d\n", (int) b); }

    void print(const char* str)
    {
        if (_colon) {
            printf("%c%c:%c%c\n", str[0], str[1], str[2], str[3]);
        } else {
            printf("%s\n", str);
        }
    }
      
private: 
     bool _colon = false;
 
};

class Max7219Display
{
public:
    Max7219Display(std::function<void()> scrollDone) : _scrollDone(scrollDone) { }

    void clear() { }
    void setBrightness(uint32_t b) { printf("*** Brightness set to %d\n", (int) b); }
    
    void showString(const char* str, uint32_t underscoreStart = 0, uint32_t underscoreLength = 0)
    {
        // String might start with \a or \v. Skip them
        const char* s = str;
        if (s[0] == '\a' || s[0] == '\v') {
            s += 1;
        }
        printf("\n[[ %s ]]\n\n", s);
        
        // If we're scrolling we need to call _scrollDone
        if (str[0] == '\v') {
            _scrollDone();
        }
    }

private:
    std::function<void()> _scrollDone;

};

}

#endif

