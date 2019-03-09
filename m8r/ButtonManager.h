//
//  ButtonManager.h
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

#include <m8r.h>
#include <Ticker.h>
#include <functional>
#include <vector>

namespace m8r {

	// ButtonManager class
	//
	// Handle a single button. Managed by ButtonManager

	class Button
	{
		friend class ButtonManager;
		
	public:
		// For Arduino, only Float or Pullup can be used. For ESP8266 Float or Pullup
		// can be used for all pins except GPIO16 which can be FLoat or Pulldown only
		enum class PinMode { Float, Pullup, Pulldown };
		
		Button(uint8_t pin, uint32_t id, bool activeHigh = false, PinMode = PinMode::Pullup);
		
		uint32_t id() const { return _id; }
	
	private:
		uint8_t _pin;
		uint32_t _id;
		bool _activeHigh;
		
		enum class WaitingFor { Nothing, Click, DoubleClickPress, DoubleClickRelease };
		
		WaitingFor _waitingFor;
		
		bool _pressed = false;
		int8_t _ticks = -1;
		int32_t _waitTime = -1;
	};

	// ButtonManager class
	//
	// Manage one or more buttons. Virtual handleButtonEvent method is called when an event occurs.
	// Buttons are defined separately and assigned with addButton().
	//
	// Timing values:
	//
	//		TickMultiplier	- number of ticks per DebounceTime
	//		DebounceTime	- ms state has to be stable to indicate a press or release
	//		ClickTime		- ms within which a release must happen after a press to indicate click
	//		LongPressTime	- ms button must stay in press state to be a long press
	//
	// For double click, need first click within ClickTime, then second press must happen within
	// another ClickTime, followed by a second click within ClickTime, i.e.:
	//
	//		Press -- < ClickTime -- Release -- < ClickTime -- Press -- < ClickTime -- Release ==> send DoubleClick event
	//
	// If Button is pressed for longer than ClickTime but shorter than LongPressTime, Release is sent
	// For Click, Press is sent, followed by Click after ClickTime delay
	// For LongPress, Press is sent, followed by LongPress
	// For DoubleClick, Press is sent, followed by DoubleClick, after delay
	//		
	class ButtonManager
	{
	public:
		static constexpr int32_t TickMultiplier = 5;
		static constexpr int32_t DefaultDebounceTime = 50;
		static constexpr int32_t DefaultClickTime = 500;
		static constexpr int32_t DefaultLongPressTime = 1000;
		
		enum class Event { Press, Release, LongPress, Click };
		
		// Set a tick that is TickMultiplier times faster than debounce time.
		// We need that many times for the state to remain unchanged for a button
		// press or release to register
		ButtonManager(std::function<void(const Button&, Event)> handler, 
					  uint32_t debounceTime = DefaultDebounceTime,
					  uint32_t clickTime = DefaultClickTime,
					  uint32_t longPressTime = DefaultLongPressTime);
		
		void addButton(const Button& button) { _buttons.push_back(button); }
		
		static const ROMString& stringFromEvent(Event event);
		
	private:
		static void _fire(ButtonManager* self) { self->fire(); }
		
		void fire();
		
		std::vector<Button> _buttons;
		Ticker _ticker;
		uint32_t _msPerTick = 0;
		uint32_t _clickTime;
		uint32_t _longPressTime;
		std::function<void(const Button&, Event)> _handler;
	};
}