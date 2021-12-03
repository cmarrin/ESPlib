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

#include "mil/LocalTimeServer.h"

#include <ESP8266HTTPClient.h>
#include <JsonStreamingParser.h>
#include <time.h>

using namespace mil;

void LocalTimeServer::MyJsonListener::key(String key)
{
	switch(_state) {
		case State::None:
		if (key == "timestamp") {
			_state = State::LocalEpoch;
		} else if (key == "gmtOffset") {
			_state = State::LocalTZOffset;
		}
		break;
	}
}

void LocalTimeServer::MyJsonListener::value(String value)
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
	}
}

bool LocalTimeServer::update()
{
	bool failed = false;

	HTTPClient http;
	mil::cout << L_F("Getting time feed...\n");

	String apiURL;
	apiURL += "http://api.timezonedb.com";
	apiURL += "/v2.1/get-time-zone?key=";
	apiURL += _key;
	apiURL +="&format=json&by=zone&zone=";
	apiURL += _city;

	mil::cout << L_F("URL='") << apiURL << L_F("'\n");

	http.setReuse(true);
	http.setTimeout(10000);
	int httpCode = 0;
	
	for (int count = 0; count < 10; ++count) {
		http.begin(apiURL);
		httpCode = http.GET();
		if (httpCode == 0) {
			delay(1000);
			mil::cout << "no response from time server, retrying...\n";
		} else {
			break;
		}
	}

	if (httpCode > 0) {
		mil::cout << L_F("    got response: ") << httpCode << L_F("\n");

		if(httpCode == HTTP_CODE_OK) {
			String payload = http.getString();
			mil::cout << L_F("Got payload, parsing...\n");
			JsonStreamingParser parser;
			MyJsonListener listener;
			parser.setListener(&listener);
			for (int i = 0; i < payload.length(); ++i) {
				parser.parse(payload.c_str()[i]);
			}
	
			_currentTime = listener.localEpoch();
		}
	} else {
		mil::cout << L_F("[HTTP] GET... failed, error: ") << http.errorToString(httpCode) << L_F("(") << httpCode << L_F(")\n");
		failed = true;
	}

	http.end();

	// Check one minute past the hour. That way if daylight savings changes, we catch it sooner
	int32_t timeToNextCheck = failed ? 120 : ((60 * 60) - (static_cast<int32_t>(_currentTime % (60 * 60))) + 60);
	_ticker.once(timeToNextCheck, fire, this);
	
	mil::cout << L_F("Time set to:") << strftime("%a %b %d, %Y %r", currentTime()) << 
		L_F(", next setting in ") << timeToNextCheck << L_F(" seconds\n");
	return !failed;
}

String LocalTimeServer::strftime(const char* format, const struct tm& time)
{
	char s[100];
	std::strftime(s, 99, format, &time);
	return s;
}

String LocalTimeServer::strftime(const char* format, uint32_t time)
{
	struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&time));
	return strftime(format, *timeinfo);
}

String LocalTimeServer::prettyDay(uint32_t time)
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
