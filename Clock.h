/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include "TimeWeatherServer.h"
#include "System.h"

#include <assert.h>
#include <time.h>

namespace mil {

class Application;

class Clock
{
public:
	Clock(Application* app);
	
	uint32_t currentTime() { return _currentTime; }
	std::string strftime(const char* format, uint32_t time) { return _timeWeatherServer.strftime(format, time); }
	std::string prettyDay(uint32_t time) { return _timeWeatherServer.prettyDay(time); }
    std::string prettyTime();
	const char* weatherConditions() { return _timeWeatherServer.conditions(); }
	uint32_t currentTemp() { return _timeWeatherServer.currentTemp(); }
	uint32_t highTemp() { return _timeWeatherServer.highTemp(); }
	uint32_t lowTemp() { return _timeWeatherServer.lowTemp(); }

	void setup();
	void loop();
	
private:
    Application* _app = nullptr;
	mil::TimeWeatherServer _timeWeatherServer;
	Ticker _secondTimer;
	
	uint32_t _currentTime = 0;
	bool _needsUpdate = true;
	
    std::string _customMenuHTML;
};

}
