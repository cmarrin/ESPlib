//
//  WUnderground.cpp
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

#include "m8r/WUnderground.h"

#include <ESP8266HTTPClient.h>
#include <JsonStreamingParser.h>
#include <time.h>

using namespace m8r;

void MyJsonListener::key(String key)
{
	switch(_state) {
		case State::None:
		if (key == "local_epoch") {
			_state = State::LocalEpoch;
		} else if (key == "local_tz_offset") {
			_state = State::LocalTZOffset;
		} else if (key == "temp_f") {
			_state = State::CurrentTemp;
		} else if (key == "simpleforecast") {
			_state = State::SimpleForecast;
		}
		break;
		
		case State::SimpleForecast:
		if (key == "period") {
			_state = State::Period;
		}
		break;

		case State::High:
		if (key == "high") {
			_state = State::HighF;
		}
		break;

		case State::HighF:
		if (key == "fahrenheit") {
			_state = State::HighFValue;
		}
		break;

		case State::Low:
		if (key == "low") {
			_state = State::LowF;
		}
		break;

		case State::LowF:
		if (key == "fahrenheit") {
			_state = State::LowFValue;
		}
		break;

		case State::Conditions:
		if (key == "conditions") {
			_state = State::ConditionsValue;
		}
		break;
	}
}

void MyJsonListener::value(String value)
{
	switch(_state) {
		case State::LocalEpoch: 
		_localEpoch = value.toInt();
		_state = State::None;
		break;
		
		case State::LocalTZOffset: 
		_localTZOffset = value.toInt();
		_state = State::None;
		break;
		
		case State::CurrentTemp: 
		_currentTemp = value.toInt();
		_state = State::None;
		break;
		
		case State::Period: {
			int32_t v = value.toInt();
			if (v != 1) {
				_state = State::SimpleForecast;
			} else {
				_state = State::High;
			}
			break;
		}
		
		case State::HighFValue: 
		_highTemp = value.toInt();
		_state = State::Low;
		break;
		
		case State::LowFValue: 
		_lowTemp = value.toInt();
		_state = State::Conditions;
		break;
		
		case State::ConditionsValue: 
		_conditions = value;
		_state = State::None;
		break;
	}
}

bool WUnderground::update()
{
	bool failed = false;

	HTTPClient http;
	m8r::cout << L_F("Getting weather and time feed...\n");

	String wuURL;
	wuURL += L_F("http://api.wunderground.com/api/");
	wuURL += _key;
	wuURL += L_F("/conditions/forecast/q/");
	wuURL += _state;
	wuURL += L_F("/");
	wuURL += _city;
	wuURL += L_F(".json?a=");
	wuURL += millis();

	m8r::cout << L_F("URL='") << wuURL << L_F("'\n");

	http.begin(wuURL);
	int httpCode = http.GET();

	if (httpCode > 0) {
		m8r::cout << L_F("    got response: ") << httpCode << L_F("\n");

		if(httpCode == HTTP_CODE_OK) {
			String payload = http.getString();
			m8r::cout << L_F("Got payload, parsing...\n");
			JsonStreamingParser parser;
			MyJsonListener listener;
			parser.setListener(&listener);
			for (int i = 0; i < payload.length(); ++i) {
				parser.parse(payload.c_str()[i]);
			}
	
			_currentTime = listener.localEpoch() + (listener.localTZOffset() * 3600 / 100);
			_currentTemp = listener.currentTemp();
			_lowTemp = listener.lowTemp();
			_highTemp = listener.highTemp();
			_conditions = listener.conditions();
		}
	} else {
		m8r::cout << L_F("[HTTP] GET... failed, error: ") << http.errorToString(httpCode) << L_F("(") << httpCode << L_F(")\n");
		failed = true;
	}

	http.end();

	// Check one minute past the hour. That way if daylight savings changes, we catch it sooner
	int32_t timeToNextCheck = failed ? 120 : ((60 * 60) - (static_cast<int32_t>(_currentTime % (60 * 60))) + 60);
	_ticker.once(timeToNextCheck, fire, this);
	
	m8r::cout << L_F("Time set to:") << strftime("%a %b %d, %Y %r", currentTime()) << 
		L_F(", next setting in ") << timeToNextCheck << L_F(" seconds\n");
	m8r::cout << "Temps - current:" << _currentTemp << ", low:" << _lowTemp << ", high:" << _highTemp << "\n";
	return !failed;
}

String WUnderground::strftime(const char* format, uint32_t time)
{
	struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&time));
	char s[100];
	std::strftime(s, 99, format, timeinfo);
	return s;
}

String WUnderground::prettyDay(uint32_t time)
{
	struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&time));
	int day = timeinfo->tm_mday;
	String s(day);
	switch(day) {
		case 1:
		case 21:
		case 31: return s + "st";
		case 2:
		case 22: return s + "nd";
		case 3:
		case 23: return s + "rd";
		default: return s + "th";
	}
}
