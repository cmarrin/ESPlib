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
        app->addParam("zipcode", "Zipcode/Postal Code, City Name, IP Address or Lat/Long (e.g., 54.851019,-8.140025)", "00000",  MaxZipCodeLength);
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

void Clock::loop()
{
	if (_needsUpdate) {
		_needsUpdate = false;
		
		if (_app && _app->isNetworkEnabled()) {
			if (_timeWeatherServer.update(_app->getParamValue("zipcode"))) {
				_currentTime = _timeWeatherServer.currentTime();
				_app->sendInput(Input::Idle, false);
			} else {
				_app->sendInput(Input::UpdateFail, false);
			}
		}
	}
}
