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

#include "TimeWeatherServer.h"

#ifdef ARDUINO
#if defined(ESP8266)
#include <ESP8266HWiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif
#else
#include <curl/curl.h>
#endif

#include "JsonStreamingParser.h"

using namespace mil;

void TimeWeatherServer::MyJsonListener::key(const CPString& key)
{
	switch(_state) {
        default: break;
		case State::None:
        if (key == "timestamp") {
            _state = State::Timestamp;
        } else if (key == "location") {
			_state = State::Location;
		} else if (key == "current") {
			_state = State::Current;
		} else if (key == "forecast") {
			_state = State::Forecast;
		}
		break;
        case State::Location:
        if (key == "tz_id") {
            _state = State::TimeZone;
        }
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

void TimeWeatherServer::MyJsonListener::value(const CPString& value)
{
	switch(_state) {
        default: break;
        case State::Timestamp:
        _currentTime = uint32_t(atol(value.c_str()));
        _state = State::None;
        break;
        case State::TimeZone:
        _timeZone = value;
        _state = State::Location;
        break;
		case State::ConditionText: 
		_conditions = value;
		_state = State::Current;
		break;
		case State::CurrentTemp: 
		_currentTemp = int32_t(ToFloat(value) + 0.5);
		_state = State::Current;
		break;
		case State::MinTemp:
		_lowTemp = int32_t(ToFloat(value) + 0.5);
		_state = State::Day;
		break;
		case State::MaxTemp: 
		_highTemp = int32_t(ToFloat(value) + 0.5);
		_state = State::Day;
		break;
	}
}

void TimeWeatherServer::MyJsonListener::endObject()
{
	_state = State::None;
}

#ifndef ARDUINO
// HTTP callback for Mac
static size_t httpCB(char* p, size_t size, size_t nmemb, void* parser)
{
    size *= nmemb;
    for (size_t i = 0; i < size; ++i) {
        reinterpret_cast<JsonStreamingParser*>(parser)->parse(p[i]);
    }
    return size;
}
#endif



//		static const constexpr char* TimeAPIKey = "OFTZYMX4MSPG";
//
//
//	String apiURL;
//	apiURL += "http://api.timezonedb.com";
//	apiURL += "/v2.1/get-time-zone?key=";
//	apiURL += TimeAPIKey;
//	apiURL +="&format=json&by=zone&zone=";
//	apiURL += _city;
//
//
//
//http://api.timezonedb.com/v2.1/get-time-zone?key=OFTZYMX4MSPG&format=json&by=position&lat=35.2783&lng=-120.692
//http://api.timezonedb.com/v2.1/get-time-zone?key=OFTZYMX4MSPG&format=json&by=zone&zone=America/Los_Angeles
//1732379310

bool
TimeWeatherServer::fetchAndParse(const char* url, JsonStreamingParser* parser)
{
    bool success = true;
    
#ifdef ARDUINO
    WiFiClient client;
	HTTPClient http;

	http.begin(client, url);
	int httpCode = http.GET();

	if (httpCode > 0) {
		cout << F("    got response: ") << int32_t(httpCode) << F("\n");

		if(httpCode == HTTP_CODE_OK) {
			String payload = http.getString();
			cout << F("Got payload, parsing...\n");
			for (int i = 0; i < payload.length(); ++i) {
				parser->parse(payload.c_str()[i]);
			}
		}
	} else {
		cout << F("[HTTP] GET... failed, error: ") << http.errorToString(httpCode) << F("(") << int32_t(httpCode) << F(")\n");
		success = false;
	}

	http.end();
#else
    CURL* curl = curl_easy_init();
    
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
 
        // Use HTTP/3 but fallback to earlier HTTP if necessary
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_3);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, parser);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpCB);
         
        // Perform the request, res gets the return code
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << "\n";
        }
        
        // always cleanup
        curl_easy_cleanup(curl);
    }
#endif
    return success;
}

bool TimeWeatherServer::update()
{
	bool failed = false;

	cout << F("Getting weather feed...\n");

	CPString apiURL;
	apiURL = "http://api.weatherapi.com/v1/forecast.json?key=";
	apiURL += WeatherAPIKey;
	apiURL +="&q=";
	apiURL += _zip;
	apiURL +="&days=1";

	cout << F("URL='") << apiURL.c_str() << F("'\n");

    JsonStreamingParser parser;
    MyJsonListener listener;
    parser.setListener(&listener);

    if (fetchAndParse(apiURL.c_str(), &parser)) {
        _currentTemp = listener._currentTemp;
        _lowTemp = listener._lowTemp;
        _highTemp = listener._highTemp;
        _conditions = listener._conditions;
    }
    
	cout << F("Getting time feed...\n");

	apiURL = "http://api.timezonedb.com";
	apiURL += "/v2.1/get-time-zone?key=";
	apiURL += TimeAPIKey;
	apiURL +="&format=json&by=zone&zone=";
	apiURL += listener._timeZone;

    parser.reset();
    if (fetchAndParse(apiURL.c_str(), &parser)) {
        _currentTime = listener._currentTime;
    }
    
	// Check every hour
	int32_t timeToNextCheck = 60 * 60 * 1000;
	_ticker.once_ms(timeToNextCheck, [this]() { _handler(); });
	
	cout << F("Epoch: ") << _currentTime << F("\nWeather: conditions='") << _conditions.c_str() << 
	 			 F("'\n    currentTemp=") << _currentTemp << 
		 		 F(", lowTemp=") << _lowTemp << 
				 F(", highTemp=") << _highTemp << 
				 F(", next setting in ") << timeToNextCheck << 
				 F(" seconds\n");
	return !failed;
}

CPString TimeWeatherServer::strftime(const char* format, const struct tm& time)
{
	char s[100];
	std::strftime(s, 99, format, &time);
	return s;
}

CPString TimeWeatherServer::strftime(const char* format, uint32_t time)
{
    time_t t = time;
	struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
	return strftime(format, timeinfo);
}

CPString TimeWeatherServer::prettyDay(uint32_t time)
{
    time_t t = time;
	struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
	int day = timeinfo.tm_mday;
	CPString s = ToString(day);
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
