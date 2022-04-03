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

#include "mil/WeatherServer.h"

#include <ESP8266HTTPClient.h>
#include <JsonStreamingParser.h>

using namespace mil;

void WeatherServer::MyJsonListener::key(String key)
{
	switch(_state) {
		case State::None:
		if (key == "current") {
			_state = State::Current;
		} else if (key == "forecast") {
			_state = State::Forecast;
		}
		break;
		case State::Current:
		if (key == "temp_f") {
			_state = State::CurrentTemp;
		} else if (key == "condition") {
			_state = State::Condition;
		}
		case State::Condition:
		if (key == "text") {
			_state = State::ConditionText;
		}
		case State::Forecast:
		if (key == "forecastday") {
			_state = State::ForecastDay;
		}
		case State::ForecastDay:
		if (key == "day") {
			_state = State::Day;
		}
		case State::Day:
		if (key == "maxtemp_f") {
			_state = State::MaxTemp;
		} else if (key == "mintemp_f") {
			_state = State::MinTemp;
		}
	}
}

void WeatherServer::MyJsonListener::value(String value)
{
	switch(_state) {
		case State::ConditionText: 
		_conditions = value;
		_state = State::Current;
		break;
		case State::CurrentTemp: 
		_currentTemp = value.toFloat() + 0.5;
		_state = State::Current;
		break;
		case State::MinTemp:
		_lowTemp = value.toFloat() + 0.5;
		_state = State::Day;
		break;
		case State::MaxTemp: 
		_highTemp = value.toFloat() + 0.5;
		_state = State::Day;
		break;
	}
}

void WeatherServer::MyJsonListener::endObject()
{
	_state = State::None;
}

bool WeatherServer::update()
{
	bool failed = false;

    WiFiClient client;
	HTTPClient http;
	mil::cout << F("Getting weather feed...\n");

	String apiURL;
	apiURL += "http://api.weatherapi.com/v1/forecast.json?key=";
	apiURL += WeatherAPIKey;
	apiURL +="&q=";
	apiURL += _zip;
	apiURL +="&days=1";

	mil::cout << F("URL='") << apiURL << F("'\n");

	http.begin(client, apiURL);
	int httpCode = http.GET();

	if (httpCode > 0) {
		mil::cout << F("    got response: ") << httpCode << F("\n");

		if(httpCode == HTTP_CODE_OK) {
			String payload = http.getString();
			mil::cout << F("Got payload, parsing...\n");
			JsonStreamingParser parser;
			MyJsonListener listener;
			parser.setListener(&listener);
			for (int i = 0; i < payload.length(); ++i) {
				parser.parse(payload.c_str()[i]);
			}
	
			_currentTemp = listener._currentTemp;
			_lowTemp = listener._lowTemp;
			_highTemp = listener._highTemp;
			_conditions = listener._conditions;
		}
	} else {
		mil::cout << F("[HTTP] GET... failed, error: ") << http.errorToString(httpCode) << F("(") << httpCode << F(")\n");
		failed = true;
	}

	http.end();

	// Check every hour
	int32_t timeToNextCheck = 60 * 60;
	_ticker.once(timeToNextCheck, fire, this);
	
	mil::cout << F("Weather: conditions='") << _conditions << 
	 			 F("', currentTemp=") << _currentTemp << 
		 		 F("', lowTemp=") << _lowTemp << 
				 F("', highTemp=") << _highTemp << 
				 F("', next setting in ") << timeToNextCheck << 
				 F(" seconds\n");
	return !failed;
}
