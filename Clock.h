/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// Arduino IDE Setup:
//
// Board: LOLIN(WEMOS) D1 R2 & mini

// Clock is the base class for ESP based clocks. It supports an ambient light sensor
// on AO and a button on a passed in port number (D1 by default)
//
// It uses the mil::LocalTimeServer to get the time and mil::WeatherServer for
// current conditions, current, low and high temps
//
// Subclasses must implement the display function to output text to the display.


// Pinout
//
//      A0 - Light sensor
//
// Light sensor
//
//                             A0
//                             |
//                             |
//		3.3v -------- 47KΩ ----|---- Sensor -------- GND
//                                   ^
//                                   |
//                             Longer Lead
//

#include "mil.h"
#include "TimeWeatherServer.h"

#include <assert.h>
#include <time.h>

namespace mil {

class Application;

class Clock
{
public:
	Clock(Application* app, const char* zipCode);
	
	uint32_t currentTime() { return _currentTime; }
	CPString strftime(const char* format, uint32_t time) { return _timeWeatherServer.strftime(format, time); }
	CPString prettyDay(uint32_t time) { return _timeWeatherServer.prettyDay(time); }
	const char* weatherConditions() { return _timeWeatherServer.conditions(); }
	uint32_t currentTemp() { return _timeWeatherServer.currentTemp(); }
	uint32_t highTemp() { return _timeWeatherServer.highTemp(); }
	uint32_t lowTemp() { return _timeWeatherServer.lowTemp(); }

	void setup();
	void loop();
	
private:
	mil::TimeWeatherServer _timeWeatherServer;
	Ticker _secondTimer;
	
	uint32_t _currentTime = 0;
	bool _needsUpdate = true;
	
	struct tm  _settingTime;
	bool _settingTimeChanged = false;
	
    Application* _app = nullptr;
};

}
