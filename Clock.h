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
#include "Blinker.h"
#include "BrightnessManager.h"
#include "ButtonManager.h"
#include "TimeWeatherServer.h"

#include <assert.h>
#include <time.h>

namespace mil {

// All rates in ms

// BrightnessManager settings
static constexpr uint32_t NumberOfBrightnessLevels = 250;

using BrightnessChangeCB = std::function<void(uint32_t brightness)>;

class Application;

class Clock
{
public:
	Clock(Application* app, const char* zipCode,
		  uint8_t lightSensor, bool invertAmbientLightLevel, uint32_t minBrightness, uint32_t maxBrightness,
		  uint8_t button, BrightnessChangeCB);
	
	uint64_t currentTime() { return _currentTime; }
	std::string strftime(const char* format, uint32_t time) { return _timeWeatherServer.strftime(format, time); }
	std::string prettyDay(uint64_t time) { return _timeWeatherServer.prettyDay(time); }
	const char* weatherConditions() { return _timeWeatherServer.conditions(); }
	uint32_t currentTemp() { return _timeWeatherServer.currentTemp(); }
	uint32_t highTemp() { return _timeWeatherServer.highTemp(); }
	uint32_t lowTemp() { return _timeWeatherServer.lowTemp(); }

    void setBrightness(uint8_t b) { _brightnessChangeCB(b); }
	void setup();
	void loop();
	
private:
	void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
	void handleBrightnessChange(uint32_t brightness);
	
	mil::ButtonManager _buttonManager;
	mil::TimeWeatherServer _timeWeatherServer;
	mil::BrightnessManager _brightnessManager;
	Ticker _secondTimer;
	
	uint64_t _currentTime = 0;
	bool _needsUpdate = true;
	
	struct tm  _settingTime;
	bool _settingTimeChanged = false;
	
	uint8_t _button = 1;
 
    BrightnessChangeCB _brightnessChangeCB;
     
    Application* _app = nullptr;
};

}
