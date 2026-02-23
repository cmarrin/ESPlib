/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Clock.h"

#include "Application.h"

using namespace mil;

static constexpr int MaxZipCodeLength = 40;

Clock::Clock(Application* app)
        : _app(app)
		, _timeWeatherServer([this]() { _needsUpdate = true; })
	{
        app->addParam("zipcode", "Zipcode/Postal Code, City Name, IP Address or Lat/Long (e.g., 54.851019,-8.140025)", "93405",  MaxZipCodeLength);
		memset(&_settingTime, 0, sizeof(_settingTime));
		_settingTime.tm_mday = 1;
		_settingTime.tm_year = 100;
	}
	
void Clock::setup()
{
	_secondTimer.attach_ms(1000, [this]() {
        _currentTime++;
        _app->sendInput(Input::Idle, true);
    });
}

std::string
Clock::prettyTime(uint32_t time)
{
    std::string s;
    
    s += strftime("%a %b ", time);
    s += prettyDay(time);
    s += " ";

    struct tm timeinfo;
    time_t ct = time;
    gmtime_r(&ct, &timeinfo);

    bool pm = false;
    uint8_t hour = timeinfo.tm_hour;

    if (hour == 0) {
        hour = 12;
    } else if (hour >= 12) {
        pm = true;
        if (hour > 12) {
            hour -= 12;
        }
    }

    s += std::to_string(hour);
    s += ":";

    uint8_t minute = timeinfo.tm_min;
    if (minute < 10) {
        s += "0";
    }
    s += std::to_string(minute);
    s += pm ? "pm" : "am";
    
    return s;
}

void Clock::loop()
{
	if (_needsUpdate) {
		_needsUpdate = false;
		
		if (_app && _app->isNetworkEnabled()) {
            std::string zipcode;
            if (!_app->getParamValue("zipcode", zipcode)) {
                printf("*** Zipcode not set, using default\n");
                zipcode = "93405";
            }
            
			if (_timeWeatherServer.update(zipcode.c_str())) {
				_currentTime = _timeWeatherServer.currentTime();
				_app->sendInput(Input::Idle, false);
    
                // Update the status on the webpage
                _customMenuHTML = "<span style='font-size:small;margin-top:0px;'><strong>Time/Date:</strong> ";
                _customMenuHTML += prettyTime(_currentTime);
                _customMenuHTML += "</span><br><span style='font-size:small'><strong>weather:</strong> ";
                _customMenuHTML += weatherConditions();
                _customMenuHTML += "  Cur:";
                _customMenuHTML += std::to_string(currentTemp());
                _customMenuHTML += "°  Hi:";
                _customMenuHTML += std::to_string(highTemp());
                _customMenuHTML += "°  Lo:";
                _customMenuHTML += std::to_string(lowTemp());
                _customMenuHTML += "°</span><br><br>";
                
                _app->setCustomMenuHTML(_customMenuHTML.c_str());
			} else {
				_app->sendInput(Input::UpdateFail, false);
			}
		}
	}
}
