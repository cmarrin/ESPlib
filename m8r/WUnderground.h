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
#include <ESP8266HTTPClient.h>
#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include <Ticker.h>
#include <time.h>

// WUnderground
//
// Get the local weather feed from Weather Underground. Parses the current time 
// and date as well as the current, high and low temps for the area

namespace m8r {

	class MyJsonListener : public JsonListener
	{
	public:
		virtual ~MyJsonListener() { }
	
		virtual void key(String key) override
		{
			if (key == "local_epoch") {
				_waitingForLocalEpoch = true;
			} else if (key == "local_tz_offset") {
				_waitingForLocalTZOffset = true;
			}
		}
	
		virtual void value(String value) override
		{
			if (_waitingForLocalEpoch) {
				_waitingForLocalEpoch = false;
				_localEpoch = value.toInt();
			} else if (_waitingForLocalTZOffset) {
				_waitingForLocalTZOffset = false;
				_localTZOffset = value.toInt();
			} 
		}
	
		virtual void whitespace(char c) override { }
		virtual void startDocument() override { }
		virtual void endArray() override { }
		virtual void endObject() override { }
		virtual void endDocument() override { }
		virtual void startArray() override { }
		virtual void startObject() override { }
	
		uint32_t localEpoch() const { return _localEpoch; }
		int32_t localTZOffset() const { return _localTZOffset; }
	
	private:
		bool _waitingForLocalEpoch = false;
		uint32_t _localEpoch = 0;
		bool _waitingForLocalTZOffset = false;
		int32_t _localTZOffset = 0;
	};

	class WUnderground
	{
	public:
		WUnderground(const String& key, const String& city, const String& state)
			: _key(key)
			, _city(city)
			, _state(state)
		{
		}
		
		virtual void handleWeatherInfo(bool succeeded) = 0;
		
		uint32_t currentTime() const { return _currentTime; }
		int8_t currentTemp() const { return _currentTemp; }
		int8_t lowTemp() const { return _lowTemp; }
		int8_t highTemp() const { return _highTemp; }
	
		void feedWUnderground()
		{
			if (!_needUpdate) {
				return;
			}
			
			_needUpdate = false;
			
			bool failed = false;
	
			HTTPClient http;
			m8r::cout << "Getting weather and time feed...\n";

			String wuURL;
			wuURL += "http://api.wunderground.com/api/";
			wuURL += _key;
			wuURL +="/conditions/forecast/q/";
			wuURL += _state;
			wuURL += "/";
			wuURL += _city;
			wuURL += ".json?a=";
			wuURL += millis();
	
			m8r::cout << "URL='" << wuURL << "'\n";
	
			http.begin(wuURL);
			int httpCode = http.GET();

			if (httpCode > 0) {
				m8r::cout << "    got response: " << httpCode << "\n";

				if(httpCode == HTTP_CODE_OK) {
					String payload = http.getString();
					m8r::cout << "Got payload, parsing...\n";
					JsonStreamingParser parser;
					MyJsonListener listener;
					parser.setListener(&listener);
					for (int i = 0; i < payload.length(); ++i) {
						parser.parse(payload.c_str()[i]);
					}
			
					_currentTime = listener.localEpoch() + (listener.localTZOffset() * 3600 / 100);
				}
			} else {
				m8r::cout << "[HTTP] GET... failed, error: " << http.errorToString(httpCode) << "(" << httpCode << ")\n";
				failed = true;
			}

			http.end();
	
			// Check one minute past the hour. That way if daylight savings changes, we catch it sooner
			int32_t timeToNextCheck = failed ? 120 : ((60 * 60) - (static_cast<int32_t>(_currentTime % (60 * 60))) + 60);
			_ticker.once(timeToNextCheck, fire, this);
			
			handleWeatherInfo(!failed);
	
			m8r::cout << "Time set to:" << 
				strtok(ctime(reinterpret_cast<time_t*>(&_currentTime)), "\n") << 
				", next setting in " << timeToNextCheck << " seconds\n";
		}
		
	private:
		static void fire(WUnderground* self) { self->_needUpdate = true; }

		String _key;
		String _city;
		String _state;
		Ticker _ticker;
		
		uint32_t _currentTime = 0;
		int8_t _currentTemp = 0;
		int8_t _lowTemp = 0;
		int8_t _highTemp = 0;
		
		bool _needUpdate = true;
	};

}