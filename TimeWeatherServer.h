/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// The Time/Weather server accesses several servers to get the current
// time and today's weather forecast. The process is as follows:
//
//  - Use the openweathermap geolocation api to get the current lat/long from zipcode
//      (ex: http://api.openweathermap.org/geo/1.0/zip?zip=93405&appid=f561ef7b384f3a56cb541c505b252e3a)
//  - Use the lat/long to get the weather forecast from weatherapi
//      (ex: https://api.weatherapi.com/v1/forecast.json?key=4a5c6eaf78d449f88d5182555210312&q=35.2901,-120.6817)
//  - Use the lat/long to get the current time from timezonedb
//      (ex: http://api.timezonedb.com/v2.1/get-time-zone?key=OFTZYMX4MSPG&format=json&by=position&lat=35.2901&lng=-120.6817)
//

#pragma once

#include "System.h"

#include <ctime>
#include <optional>

// TimeWeatherServer
//
// Get the local time and weather conditions

static constexpr uint32_t UpdateFrequency = 60 * 60; // in seconds

class JsonStreamingParser;

namespace mil {

class TimeWeatherServer
{
  public:
    TimeWeatherServer(std::function<void()> handler)
        : _handler(handler)
    {
    }
    
    uint32_t currentTime() const { return _currentTime; }
    
    static std::string strftime(const char* format, uint32_t time);
    static std::string strftime(const char* format, const struct tm&);
    static std::string prettyDay(uint32_t time);

    float latitude() const { return _latitude; }
    float longitude() const { return  _longitude; }
    
    uint32_t currentTemp() const { return _currentTemp; }
    uint32_t lowTemp() const { return _lowTemp; }
    uint32_t highTemp() const { return _highTemp; }
    const char* conditions() const { return _conditions.c_str(); }
    
    bool update(const char* zipCode);
    
  private:
    static constexpr const char* GeoLocationAPIKey = "f561ef7b384f3a56cb541c505b252e3a";
    static constexpr const char* WeatherAPIKey = "4a5c6eaf78d449f88d5182555210312";
    static const constexpr char* TimeAPIKey = "OFTZYMX4MSPG";

    Ticker _ticker;

    float _latitude = 0;
    float _longitude = 0;
    
    uint32_t _currentTime = 0;

    int32_t _currentTemp = 0;
    int32_t _lowTemp = 0;
    int32_t _highTemp = 0;
    std::string _conditions;

    std::function<void()> _handler;
};

}
