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

#include <mil.h>
#include <mil/Blinker.h>
#include <mil/BrightnessManager.h>
#include <mil/ButtonManager.h>
#include <mil/StateMachine.h>
#include <mil/LocalTimeServer.h>
#include <mil/WeatherServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <assert.h>
#include <time.h>

// All rates in ms

// Number of ms Display stays off in each mode
static constexpr uint32_t ConnectingRate = 400;
static constexpr uint32_t ConfigRate = 100;
static constexpr uint32_t ConnectedRate = 1900;
static constexpr uint32_t BlinkSampleRate = 10;

// BrightnessManager settings
static constexpr uint32_t LightSensor = A0;
static constexpr bool InvertAmbientLightLevel = true;
static constexpr uint32_t MaxAmbientLightLevel = 900;
static constexpr uint32_t MinAmbientLightLevel = 100;
static constexpr uint32_t NumberOfBrightnessLevels = 31;

// Network related
static constexpr const char* ConfigPortalName = "MT Galileo Clock";
static constexpr const char* ConfigPortalPassword = "";

// Time and weather related
MakeROMString(TimeAPIKey, "OFTZYMX4MSPG");
MakeROMString(TimeCity, "America/Los_Angeles");
MakeROMString(WeatherAPIKey, "4a5c6eaf78d449f88d5182555210312");
MakeROMString(WeatherCity, "93405");

// Button
static constexpr uint8_t SelectButton = D1;

enum class State {
	Connecting, NetConfig, NetFail, UpdateFail, 
	Startup, ShowInfo, ShowTime, Idle,
	AskResetNetwork, VerifyResetNetwork, ResetNetwork,
	AskRestart, Restart
};

enum class Input { Idle, SelectClick, SelectLongPress, ShowDone, Connected, NetConfig, NetFail, UpdateFail };

class Clock
{
public:
	Clock(const mil::ROMString& startupMessage);
	
	virtual void setup();
	virtual void loop();
	
	virtual void showTime(bool force = false) = 0;
	virtual void showInfo() = 0;
	virtual void showString(const String&) = 0;
	virtual void setBrightness(uint32_t) = 0;

private:
	void startNetwork();
	void startStateMachine();
	void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
	void handleBrightnessChange(uint32_t brightness);
	
	static void secondTick(Clock* self)
	{
		self->_currentTime++;
		self->_stateMachine.sendInput(Input::Idle);
	}

	mil::StateMachine<State, Input> _stateMachine;
	mil::ButtonManager _buttonManager;
	mil::LocalTimeServer _localTimeServer;
	mil::WeatherServer _weatherServer;
	mil::BrightnessManager _brightnessManager;
	mil::Blinker _blinker;
	Ticker _secondTimer;
	
	uint32_t _currentTime = 0;
	bool _needsUpdateTime = false;
	bool _needsUpdateWeather = false;
	bool _needsNetworkReset = false;
	bool _enteredConfigMode = false;
	
	struct tm  _settingTime;
	bool _settingTimeChanged = false;
	bool _enableNetwork = false;
	
	const mil::ROMString _startupMessage;
};
