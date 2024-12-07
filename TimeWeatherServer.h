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

#include "JsonListener.h"
#include <ctime>

// TimeWeatherServer
//
// Get the local time and weather conditions

class JsonStreamingParser;

namespace mil {

	class TimeWeatherServer
	{
	public:
		class MyJsonListener : public JsonListener
		{
			friend class TimeWeatherServer;
		
		public:
			virtual ~MyJsonListener() { }
	
			virtual void key(const CPString& key) override;
			virtual void value(const CPString& value) override;
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
                Timestamp,
                Location,
                    TimeZone,
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
		
			uint32_t _currentTime = 0;
			int32_t _currentTemp = 0;
			int32_t _lowTemp = 0;
			int32_t _highTemp = 0;
			CPString _conditions;
            CPString _timeZone;
		};

		TimeWeatherServer(const char* zip, std::function<void()> handler)
			: _zip(zip)
			, _handler(handler)
		{
		}
		
		uint32_t currentTime() const { return _currentTime; }
		
		static CPString strftime(const char* format, uint32_t time);
		static CPString strftime(const char* format, const struct tm&);
		static CPString prettyDay(uint32_t time);

		uint32_t currentTemp() const { return _currentTemp; }
		uint32_t lowTemp() const { return _lowTemp; }
		uint32_t highTemp() const { return _highTemp; }
		const char* conditions() const { return _conditions.c_str(); }
		
		bool update();
		
	private:
		static constexpr const char* WeatherAPIKey = "4a5c6eaf78d449f88d5182555210312";
		static const constexpr char* TimeAPIKey = "OFTZYMX4MSPG";

        bool fetchAndParse(const char* url, JsonStreamingParser*);

		CPString _zip;
		Ticker _ticker;
				
		uint32_t _currentTime = 0;

		int32_t _currentTemp = 0;
		int32_t _lowTemp = 0;
		int32_t _highTemp = 0;
		CPString _conditions;

		std::function<void()> _handler;
	};

}
