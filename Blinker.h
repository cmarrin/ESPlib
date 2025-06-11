//
//  Blinker.h
//
//  Created by Chris Marrin on 3/25/2018
//
//

/*
Copyright (c) 2009-2018 Chris Marrin (chris@marrin.com)
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

#include "mil.h"

// Blinker class
//
// Blink the LED connected to the passed pin. SampleRate passed to
// the ctor determines the rate at which the underlying ticker fires
// and the rate passed to setRate determines at what rate the LED
// blinks. The LED is on for one sampleRate time and off for the rest.
// For instance, with a sampleRate of 20ms and a rate of 100ms, the
// LED will blink 10 times a second and stay on for 20ms each time.

namespace mil {

#ifdef ARDUINO
	class Blinker
	{
	public:
		Blinker(uint8_t led, uint32_t sampleRate)
			: _led(led)
			, _sampleRate(sampleRate)
		{
			pinMode(LED_BUILTIN, OUTPUT);
			_ticker.attach_ms(sampleRate, [this]() { blink(); });
            digitalWrite(LED_BUILTIN, HIGH);
		}
	
		void setRate(uint32_t rate) { _rate = (rate + (_sampleRate / 2)) / _sampleRate; }
	
	private:
		void blink()
		{
            if (_rate == 0) {
                return;
            }
            
			if (_count == 0) {
				digitalWrite(LED_BUILTIN, LOW);
			} else if (_count == 1){
				digitalWrite(LED_BUILTIN, HIGH);
			}
			if (++_count >= _rate) {
				_count = 0;
			}
		}
	
		Ticker _ticker;
		uint32_t _rate = 0;
		uint32_t _count = 0;
		uint8_t _led;
		uint32_t _sampleRate;
	};
#else
    // Dummy on Mac
	class Blinker
	{
	public:
		Blinker(uint8_t led, uint32_t sampleRate) { }

		void setRate(uint32_t rate) { }
    };
#endif

}
