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

#include "HTTPClient.h"
#include "JsonStreamingParser.h"

using namespace mil;

void TimeWeatherServer::MyJsonListener::key(const std::string& key)
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
		break;
		case State::Current:
		if (key == "temp_f") {
			_state = State::CurrentTemp;
		} else if (key == "condition") {
			_state = State::Condition;
		}
		break;
		case State::Condition:
		if (key == "text") {
			_state = State::ConditionText;
		}
		break;
		case State::Forecast:
		if (key == "forecastday") {
			_state = State::ForecastDay;
		}
		break;
		case State::ForecastDay:
		if (key == "day") {
			_state = State::Day;
		}
		break;
		case State::Day:
		if (key == "maxtemp_f") {
			_state = State::MaxTemp;
		} else if (key == "mintemp_f") {
			_state = State::MinTemp;
		}
		break;
	}
}

void TimeWeatherServer::MyJsonListener::value(const std::string& value)
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
		_currentTemp = int32_t(std::stof(value) + 0.5);
		_state = State::Current;
		break;
		case State::MinTemp:
		_lowTemp = int32_t(std::stof(value) + 0.5);
		_state = State::Day;
		break;
		case State::MaxTemp: 
		_highTemp = int32_t(std::stof(value) + 0.5);
		_state = State::Day;
		break;
	}
}

void TimeWeatherServer::MyJsonListener::endObject()
{
	_state = State::None;
}

bool TimeWeatherServer::update(const char* zipCode)
{
	bool failed = false;

	printf("Getting geolocation feed...\n");

	std::string apiURL;
	apiURL = "http://api.openweathermap.org/geo/1.0/zip?zip=";
    apiURL += zipCode;
	apiURL +="&appid=";
	apiURL += GeoLocationAPIKey;

	printf("URL='%s'\n", apiURL.c_str());

    JsonStreamingParser parser;
    MyJsonListener listener;
    parser.setListener(&listener);
    JsonStreamingParser* p = &parser;
    
    {
        HTTPClient client([p](const char* buf, uint32_t size)
        {
            for (size_t i = 0; i < size; ++i) {
                p->parse(buf[i]);
            }
        });

        if (client.fetch(apiURL.c_str()) && listener.latitude() && listener.longitude()) {
            _latitude = *listener.latitude();
            _longitude = *listener.longitude();
        } else {
            printf("**** Failed to get geolocation data...\n");
        }
    }
bool TimeWeatherServer::update(const char* zipCode)
{
	bool failed = false;

	printf("Getting weather feed...\n");

	std::string apiURL;
	apiURL = "http://api.weatherapi.com/v1/forecast.json?key=";
	apiURL += WeatherAPIKey;
	apiURL +="&q=";
	apiURL += zipCode ? zipCode : "00000";
	apiURL +="&days=1";

	printf("URL='%s'\n", apiURL.c_str());

    JsonStreamingParser parser;
    MyJsonListener listener;
    parser.setListener(&listener);

    if (fetchAndParse(apiURL.c_str(), &parser) && listener.currentTemp() && listener.lowTemp() && listener.highTemp() && listener.conditions()) {
        _currentTemp = *listener.currentTemp();
        _lowTemp = *listener.lowTemp();
        _highTemp = *listener.highTemp();
        _conditions = *listener.conditions();
    } else {
        printf("**** Failed to get weather, using old values...\n");
    }
    
	printf("Getting time feed...\n");

	apiURL = "http://api.timezonedb.com";
	apiURL += "/v2.1/get-time-zone?key=";
	apiURL += TimeAPIKey;
	apiURL +="&format=json&by=zone&zone=";
 
	apiURL += listener.timeZone() ? *listener.timeZone() : "00000";

	printf("URL='%s'\n", apiURL.c_str());

    parser.reset();
    if (fetchAndParse(apiURL.c_str(), &parser) && listener.currentTime() && listener.timeZone()) {
        _currentTime = *listener.currentTime();
    } else {
        printf("**** Failed to get time, using old value...\n");
    }
    
	// Check at interval of UpdateFrequency seconds
	int32_t timeToNextCheck = UpdateFrequency * 1000;
	_ticker.once_ms(timeToNextCheck, [this]() { _handler(); });
 
    printf("Epoch: %u\n", (unsigned int) _currentTime);
    printf("Weather: conditions='%s'\n", _conditions.c_str());
    printf("    currentTemp=%d, lowTemp=%d, highTemp=%d, next setting in %d seconds\n", (int) _currentTemp, (int) _lowTemp, (int) _highTemp, (int) timeToNextCheck);
	return !failed;
}

std::string TimeWeatherServer::strftime(const char* format, const struct tm& time)
{
	char s[100];
	std::strftime(s, 99, format, &time);
	return s;
}

std::string TimeWeatherServer::strftime(const char* format, uint32_t time)
{
    time_t t = time;
	struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
	return strftime(format, timeinfo);
}

std::string TimeWeatherServer::prettyDay(uint32_t time)
{
    time_t t = time;
	struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
	int day = timeinfo.tm_mday;
	std::string s = std::to_string(day);
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
