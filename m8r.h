//
//  m8r.h
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

#include <Arduino.h>
#include <Printable.h>

enum ErrorConditionType { ErrorConditionNote, ErrorConditionWarning, ErrorConditionFatal };

namespace m8r {

enum ErrorType {
    AssertOutOfMem = 0x01,
    AssertPureVirtual = 0x02,
    AssertDeleteNotSupported = 0x03,
    AssertSingleApp = 0x04,
    AssertNoApp = 0x05,
    
    AssertSingleADC = 0x10,
    AssertSingleUSART0 = 0x11,
    AssertFixedPointBufferTooSmall = 0x12,
    
    AssertSingleTimer0 = 0x20,
    AssertSingleTimer1 = 0x21,
    AssertSingleTimer2 = 0x22,
    AssertSingleTimerEventMgr = 0x23,
    AssertNoTimerEventMgr = 0x24,
    AssertNoEventTimers = 0x25,
    
    AssertEthernetBadLength = 0x30,
    AssertEthernetNotInHandler = 0x31,
    AssertEthernetCannotSendData = 0x32,
    AssertEthernetNotWaitingToSendData = 0x33,
    AssertEthernetTransmitError = 0x34,
    AssertEthernetReceiveError = 0x35,
    
    AssertI2CReceiveNoBytes = 0x40,
    AssertI2CReceiveWrongSize = 0x41,
    AssertI2CSendBufferTooBig = 0x48,
    AssertI2CSendNoAckOnAddress = 0x49,
    AssertI2CSendNoAckOnData = 0x4A,
    AssertI2CSendError = 0x4B,
    
    AssertButtonTooMany = 0x50,
    AssertButtonOutOfRange = 0x51,
    AssertMenuHitEnd = 0x52,
    
    ErrorUser = 0x80,
};

class OutputStream
{
public:
	OutputStream& operator << (const char* s) { Serial.print(s); return *this; }
	OutputStream& operator << (const Printable& p) { p.printTo(Serial); return *this; }
	OutputStream& operator << (const String& s) { Serial.print(s); return *this; }
	OutputStream& operator << (int32_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint32_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (int16_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint16_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (int8_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint8_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (float v) { Serial.print(v); return *this; }
	OutputStream& operator << (double v) { Serial.print(v); return *this; }
};

extern OutputStream cout;

void _showErrorCondition(char c, uint32_t code, enum ErrorConditionType type);

#define ASSERT(expr, code) if (!(expr)) FATAL(code)
#define FATAL(code) _showErrorCondition(0, code, ErrorConditionFatal)
#define WARNING(code) _showErrorCondition(0, code, ErrorConditionWarning)

inline uint16_t makeuint16(uint8_t a, uint8_t b)
{
	return (static_cast<uint16_t>(a) << 8) | static_cast<uint16_t>(b);
}

inline void NOTE(uint16_t code) { _showErrorCondition(0, code, ErrorConditionNote); }
inline void NOTE(uint8_t code1, uint8_t code2) { _showErrorCondition(0, makeuint16(code1, code2), ErrorConditionNote); }
inline void CNOTE(char c, uint8_t code) { _showErrorCondition(c, code, ErrorConditionNote); }

}

