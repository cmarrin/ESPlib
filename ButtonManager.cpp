//
//  ButtonManager.cpp
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

#include "mil/ButtonManager.h"

using namespace mil;

Button::Button(uint8_t pin, uint32_t id, bool activeHigh, PinMode mode)
	: _pin(pin)
	, _id(id)
	, _activeHigh(activeHigh)
{
	pinMode(_pin, (mode == PinMode::Float) ? INPUT : INPUT_PULLUP);
}

ButtonManager::ButtonManager(std::function<void(const Button&, Event)> handler, uint32_t debounceTime, uint32_t clickTime, uint32_t longPressTime)
	: _clickTime(clickTime)
	, _longPressTime(longPressTime)
	, _handler(handler)
{
	_msPerTick = debounceTime / TickMultiplier;
	_ticker.attach_ms(_msPerTick, _fire, this);
}

const ROMString& ButtonManager::stringFromEvent(Event event)
{
	switch(event) {
		case Event::Press: return L_F("Press");
		case Event::Release: return L_F("Release");
		case Event::Click: return L_F("Click");
		case Event::LongPress: return L_F("LongPress");
	}	
}

void ButtonManager::fire()
{
	for (auto& it : _buttons) {
		if (it._waitTime >= 0) {
			it._waitTime += _msPerTick;
			if (it._waitTime > _longPressTime) {
				it._waitTime = -1;
				_handler(it, Event::LongPress);
			}
		}
		
		bool pressed = digitalRead(it._pin) ^ !it._activeHigh;
		if (pressed != it._pressed) {
			it._pressed = pressed;
			it._ticks = 0;
		} else if (it._ticks >= 0) {
			if (++it._ticks >= TickMultiplier) {
				// Debounce sucessful. See what we've got
				it._ticks = -1;
				if (pressed) {
					// New state is a press, send it
					_handler(it, Event::Press);
					it._waitTime = 0;
				} else {
					if (it._waitTime < _clickTime) {
						_handler(it, Event::Click);
					} else if (it._waitTime <= _longPressTime) {
						_handler(it, Event::Release);
					}
					it._waitTime = -1;
				}
				it._ticks = -1;
			}
		}
	}
}
