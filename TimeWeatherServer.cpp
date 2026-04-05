/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "TimeWeatherServer.h"

#include "HTTPFetchClient.h"
#include "JsonListener.h"
#include "JsonStreamingParser.h"

using namespace mil;

static const char* TAG = "TimeWeatherServer";

class MyJsonListener : public JsonListener
{
  public:
    virtual ~MyJsonListener() { }

    virtual void key(const std::string& key) override
    {
        switch(_state) {
            default: break;
            case State::None:
            if (key == "timestamp") {
                _state = State::Timestamp;
            } else if (key == "lat") {
                _state = State::Latitude;
            } else if (key == "lon") {
                _state = State::Longitude;
            } else if (key == "current") {
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
    
    virtual void value(const std::string& value) override
    {
        switch(_state) {
            default: break;
            case State::Timestamp:
            _currentTime = uint32_t(atol(value.c_str()));
            _state = State::None;
            break;
           case State::Latitude:
            _latitude = float(atof(value.c_str()));
            _state = State::None;
            break;
          case State::Longitude:
            _longitude = float(atof(value.c_str()));
            _state = State::None;
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
    
    virtual void whitespace(char c) override { }
    virtual void startDocument() override { }
    virtual void endArray() override { }
    virtual void endObject() override { _state = State::None; }
    virtual void endDocument() override { }
    virtual void startArray() override { }
    virtual void startObject() override { }

    std::optional<uint32_t> currentTime() const { return _currentTime; }
    std::optional<float> latitude() const { return _latitude; }
    std::optional<float> longitude() const { return _longitude; }
    std::optional<int32_t> currentTemp() const { return _currentTemp; }
    std::optional<int32_t> lowTemp() const { return _lowTemp; }
    std::optional<int32_t> highTemp() const { return _highTemp; }
    std::optional<std::string> conditions() const { return _conditions; }
    std::optional<std::string> timeZone() const { return _timeZone; }

  private:
    enum class State {
        None,
        Timestamp,
        Latitude,
        Longitude,
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

    std::optional<uint32_t> _currentTime = std::nullopt;
    std::optional<float> _longitude = std::nullopt;
    std::optional<float> _latitude = std::nullopt;
    std::optional<int32_t> _currentTemp = 0;
    std::optional<int32_t> _lowTemp = 0;
    std::optional<int32_t> _highTemp = 0;
    std::optional<std::string> _conditions;
    std::optional<std::string> _timeZone;
};

bool
TimeWeatherServer::update(const char* zipCode)
{
    std::string apiURL;
    JsonStreamingParser* parser = new JsonStreamingParser();
    MyJsonListener listener;
    parser->setListener(&listener);

    System::logI(TAG, "Getting geolocation feed...");

    apiURL = "http://api.openweathermap.org/geo/1.0/zip?zip=";
    apiURL += zipCode;
    apiURL +="&appid=";
    apiURL += GeoLocationAPIKey;

    bool failed = true;

    HTTPFetchClient client([parser](const char* buf, uint32_t size)
    {
        for (size_t i = 0; i < size; ++i) {
            parser->parse(buf[i]);
        }
    });
    
    for (int i = 0; i < 5; ++i) {
        if (client.fetch(apiURL.c_str()) && listener.latitude() && listener.longitude()) {
            _latitude = *listener.latitude();
            _longitude = *listener.longitude();
            failed = false;
            break;
        } else {
            System::logW(TAG, "Failed to get geolocation data, retrying...");
        }
    }
    
    if (!failed) {    
        System::logI(TAG, "Getting time feed...");

        apiURL = "http://api.timezonedb.com";
        apiURL += "/v2.1/get-time-zone?key=";
        apiURL += TimeAPIKey;
        apiURL +="&format=json&by=position&lat=";
        apiURL += std::to_string(_latitude);
        apiURL +="&lng=";
        apiURL += std::to_string(_longitude);
     
        apiURL += listener.timeZone() ? *listener.timeZone() : "00000";

        parser->reset();

        failed = true;

        for (int i = 0; i < 5; ++i) {
            if (client.fetch(apiURL.c_str()) && listener.currentTime()) {
                _currentTime = *listener.currentTime();
                failed = false;
                break;
            } else {
                System::logW(TAG, "Failed to get time data, retrying...");
            }
        }

        System::logI(TAG, "Epoch: %u", (unsigned int) _currentTime);
        
        System::logI(TAG, "Getting weather feed...");

        apiURL = "http://api.weatherapi.com/v1/forecast.json?key=";
        apiURL += WeatherAPIKey;
        apiURL +="&q=";
        apiURL += std::to_string(_latitude);
        apiURL +=",";
        apiURL += std::to_string(_longitude);
        apiURL +="&days=1";

        parser->reset();
        
        failed = true;
        
        for (int i = 0; i < 5; ++i) {
            if (client.fetch(apiURL.c_str()) && listener.currentTemp() && listener.lowTemp() && listener.highTemp() && listener.conditions()) {
                _currentTemp = *listener.currentTemp();
                _lowTemp = *listener.lowTemp();
                _highTemp = *listener.highTemp();
                _conditions = *listener.conditions();
                failed = false;
                break;
            } else {
                System::logW(TAG, "Failed to get weather data, retrying...");
            }
        }
        System::logI(TAG, "Weather: conditions='%s', curTemp=%d, loTemp=%d, hiTemp=%d", _conditions.c_str(), int(_currentTemp), int(_lowTemp), int(_highTemp));
    }

    
    // Check at interval of UpdateFrequency seconds
    int32_t timeToNextCheck = failed ? 60 : UpdateFrequency;
    _ticker.once_ms(timeToNextCheck * 1000, [this]() { _handler(); });
    
    if (failed) {
        System::logE(TAG, "Failed to get data");
    }
    System::logI(TAG, "Next setting in %d minutes", int(timeToNextCheck / 60));

    delete parser;
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
