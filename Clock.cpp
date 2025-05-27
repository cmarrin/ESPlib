/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Clock.h"

#include "Application.h"

using namespace mil;

Clock::Clock(Application* app, const char* zipCode)
        : _app(app)
		, _timeWeatherServer(zipCode, [this]() { _needsUpdate = true; })
	{
		memset(&_settingTime, 0, sizeof(_settingTime));
		_settingTime.tm_mday = 1;
		_settingTime.tm_year = 100;
	}
	
void Clock::setup()
{
	_secondTimer.attach_ms(10000, [this]() {
        _currentTime++;
        _app->sendInput(Input::Idle, true);
    });
}

void Clock::loop()
{
	if (_needsUpdate) {
		_needsUpdate = false;
		
		if (_app && _app->isNetworkEnabled()) {
			if (_timeWeatherServer.update()) {
				_currentTime = _timeWeatherServer.currentTime();
				_app->sendInput(Input::Idle, false);
			} else {
				_app->sendInput(Input::UpdateFail, false);
			}
		}
	}
}
