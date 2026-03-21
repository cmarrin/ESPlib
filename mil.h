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

#include <string>

#if defined ARDUINO
#include <Arduino.h>
#include <Printable.h>

#else
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <functional>

// sdkconfig.h defines values for:
//
//      CONFIG_BLINK_LED_GPIO or CONFIG_BLINK_LED_STRIP
//      CONFIG_BLINK_LED_STRIP_BACKEND_RMT or CONFIG_BLINK_LED_STRIP_BACKEND_SPI
//      BLINK_LED_GPIO_ACTIVE_LOW or BLINK_LED_GPIO_ACTIVE_HIGH
//      CONFIG_BLINK_LED_GPIO_NUMBER (number)
//
// For Mac (and Arduino?) we need to define dummy values

#if defined ESP_PLATFORM
#include "sdkconfig.h"
#endif

#if defined CONFIG_BLINK_LED_STRIP
static constexpr bool HaveAddressableRGBLED = true;
#else
static constexpr bool HaveAddressableRGBLED = false;
#endif

#if defined CONFIG_BLINK_LED_STRIP_BACKEND_RMT
static constexpr bool AddressableLEDRMT = true;
#else
static constexpr bool AddressableLEDRMT = false;
#endif

#if defined BLINK_LED_GPIO_ACTIVE_HIGH
static constexpr bool ActiveHighLED = true;
#else
static constexpr bool ActiveHighLED = false;
#endif

#if defined CONFIG_BLINK_LED_GPIO_NUMBER
static constexpr int LED_BUILTIN = CONFIG_BLINK_LED_GPIO_NUMBER;
#else
static constexpr int LED_BUILTIN = 0;
#endif

static inline void delay(uint32_t ms) { useconds_t us = useconds_t(ms) * 1000; usleep(us); }

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

}

#endif

