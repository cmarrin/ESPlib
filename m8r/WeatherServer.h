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

// WeatherServer
//
// Get the local weather conditions. Currently uses apixu.com

namespace m8r {

	class WeatherServer
	{
	public:
		class MyJsonListener : public JsonListener
		{
			friend class WeatherServer;
		
		public:
			virtual ~MyJsonListener() { }
	
			virtual void key(String key) override;
			virtual void value(String value) override;
			virtual void whitespace(char c) override { }
			virtual void startDocument() override { }
			virtual void endArray() override { }
			virtual void endObject() override;
			virtual void endDocument() override { }
			virtual void startArray() override { }
			virtual void startObject() override { }
	
		private:
			enum class State {
				None,
				Current,
					CurrentTemp,
					Condition,
						ConditionText,
				Forecast,
					ForecastDay,
						Day,
							MinTemp,	
							MaxTemp,	
			};
		
			State _state = State::None;
		
			int32_t _currentTemp = 0;
			int32_t _lowTemp = 0;
			int32_t _highTemp = 0;
			String _conditions;
		};

		WeatherServer(const String& key, const String& zip, std::function<void()> handler)
			: _key(key)
			, _zip(zip)
			, _handler(handler)
		{
		}
		
		uint32_t currentTemp() const { return _currentTemp; }
		uint32_t lowTemp() const { return _lowTemp; }
		uint32_t highTemp() const { return _highTemp; }
		const String& conditions() const { return _conditions; }
		
		bool update();
		
	private:
		static void fire(WeatherServer* self) { self->_handler(); }

		String _key;
		String _zip;
		Ticker _ticker;
				
		int32_t _currentTemp = 0;
		int32_t _lowTemp = 0;
		int32_t _highTemp = 0;
		String _conditions;

		std::function<void()> _handler;
	};

}