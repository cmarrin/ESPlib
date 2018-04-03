//
//  WUnderground.h
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
#include <JsonListener.h>
#include <Ticker.h>
#include <ctime>

// WUnderground
//
// Get the local weather feed from Weather Underground. Parses the current time 
// and date as well as the current, high and low temps for the area

namespace m8r {

	class MyJsonListener : public JsonListener
	{
	public:
		virtual ~MyJsonListener() { }
	
		virtual void key(String key) override;
		virtual void value(String value) override;
		virtual void whitespace(char c) override { }
		virtual void startDocument() override { }
		virtual void endArray() override { }
		virtual void endObject() override { }
		virtual void endDocument() override { }
		virtual void startArray() override { }
		virtual void startObject() override { }
	
		uint32_t localEpoch() const { return _localEpoch; }
		int32_t localTZOffset() const { return _localTZOffset; }
		int32_t currentTemp() const { return _currentTemp; }
		int32_t lowTemp() const { return _lowTemp; }
		int32_t highTemp() const { return _highTemp; }
		const String& conditions() const { return _conditions; }
	
	private:
		// LocalEpoch, LocalTZOffset and CurrentTemp are all independent. When we see the string we wait for the
		// next entity, which will be the value. For forecast temps we wait for the sequence:
		//
		//		"forecast" -> "simpleforecast" -> "period" -> <value == 1> -> <"high" | "low"> -> "fahrenheit" -> <value>
		// If the period value is anything other that "1", we go back to the Period state
		enum class State {
			None, LocalEpoch, LocalTZOffset, CurrentTemp, 
			SimpleForecast, Period,
			Low, High, LowF, HighF, LowFValue, HighFValue,
			Conditions, ConditionsValue
		};
		
		State _state = State::None;
		
		uint32_t _localEpoch = 0;
		int32_t _localTZOffset = 0;
		int32_t _currentTemp = -1000;
		int32_t _lowTemp = -1000;
		int32_t _highTemp = -1000;
		String _conditions;
	};

	class WUnderground
	{
	public:
		WUnderground(const String& key, const String& city, const String& state, std::function<void(bool succeeded)> handler)
			: _key(key)
			, _city(city)
			, _state(state)
			, _handler(handler)
		{
		}
		
		uint32_t currentTime() const { return _currentTime; }
		int8_t currentTemp() const { return _currentTemp; }
		int8_t lowTemp() const { return _lowTemp; }
		int8_t highTemp() const { return _highTemp; }
		const String& conditions() const { return _conditions; }
		
		static String strftime(const char* format, uint32_t time);
		static String prettyDay(uint32_t time);
	
		void feed();
		
	private:
		static void fire(WUnderground* self) { self->_needUpdate = true; }

		String _key;
		String _city;
		String _state;
		Ticker _ticker;
		
		uint32_t _currentTime = 0;
		int32_t _currentTemp = 0;
		int32_t _lowTemp = 0;
		int32_t _highTemp = 0;
		String _conditions;
		
		bool _needUpdate = true;

		std::function<void(bool succeeded)> _handler;
	};

}