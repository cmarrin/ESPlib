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

using namespace m8r;

void MyJsonListener::key(String key)
{
	if (key == "local_epoch") {
		_waitingForLocalEpoch = true;
	} else if (key == "local_tz_offset") {
		_waitingForLocalTZOffset = true;
	}
}

void MyJsonListener::value(String value)
{
	if (_waitingForLocalEpoch) {
		_waitingForLocalEpoch = false;
		_localEpoch = value.toInt();
	} else if (_waitingForLocalTZOffset) {
		_waitingForLocalTZOffset = false;
		_localTZOffset = value.toInt();
	} 
}

void WUnderground::feedWUnderground()
{
	if (!_needUpdate) {
		return;
	}
	
	_needUpdate = false;
	
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
		}
	} else {
		m8r::cout << L_F("[HTTP] GET... failed, error: ") << http.errorToString(httpCode) << L_F("(") << httpCode << L_F(")\n");
		failed = true;
	}

	http.end();

	// Check one minute past the hour. That way if daylight savings changes, we catch it sooner
	int32_t timeToNextCheck = failed ? 120 : ((60 * 60) - (static_cast<int32_t>(_currentTime % (60 * 60))) + 60);
	_ticker.once(timeToNextCheck, fire, this);
	
	handleWeatherInfo(!failed);

	m8r::cout << L_F("Time set to:") << 
		strtok(ctime(reinterpret_cast<time_t*>(&_currentTime)), "\n") << 
		L_F(", next setting in ") << timeToNextCheck << L_F(" seconds\n");
}
