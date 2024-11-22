/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// Application is the main class for ESP based projects. It uses the 
// mil::TimeWeatherServer to get the time current weather conditions.
//
// It also sets up the WiFi network. On the first time through or when
// network settings have been reset, it uses WiFiManager to setup a captive
// network to let the user set the ssid and password for the network. Then
// it restarts and connects to the local network. On subsequent runs it
// connects directly to the local network.
//
// A Blinker is used to indicate the network status on the on-board LED
// if any. If one is available, its pin number is provided to the 
// Application constructor.
//
// It has a state machine which managers the network status and allows
// the network to be restart or reset. The main app tells Application
// to go into the network restart or network reset state. This is 
// typically done with a pushbutton managed by the main app. For instance
// a long press of the button will enter the Restart state, a second
// long press will enter the Reset state. In either of these states
// a long press will activate restart or reset.
//
// The state machine sends messages to the main app to show information
// about the current network status.

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

namespace mil {

// All rates in ms

// Number of ms Display stays off in each mode
static constexpr uint32_t ConnectingRate = 400;
static constexpr uint32_t ConfigRate = 100;
static constexpr uint32_t ConnectedRate = 1900;
static constexpr uint32_t BlinkSampleRate = 4;

enum class State {
	Connecting, NetConfig, NetFail, UpdateFail, 
	Startup, ShowInfo, ForceShowTime, ShowTime, Idle,
	AskResetNetwork, VerifyResetNetwork, ResetNetwork,
	AskRestart, Restart
};

enum class Input { Idle, SelectClick, SelectLongPress, ShowDone, Connected, NetConfig, NetFail, UpdateFail };

enum class Message { Startup, Connecting, NetConfig, NetFail, UpdateFail, AskRestart, AskResetNetwork, VerifyResetNetwork };

class Clock
{
public:
	Clock(const String& timeCity, const String& weatherCity,
		  bool invertAmbientLightLevel, uint32_t minBrightness, uint32_t maxBrightness,
		  uint8_t button, const String& configPortalName);
	
	virtual void setup();
	virtual void loop();
	
	virtual void showTime(bool force = false) = 0;
	virtual void showInfo() = 0;
	virtual void showString(Message) = 0;
	virtual void setBrightness(uint32_t) = 0;
	
	void startShowDoneTimer(uint32_t ms);
	
	uint32_t currentTime() { return _currentTime; }
	String strftime(const char* format, uint32_t time) { return _localTimeServer.strftime(format, time); }
	String prettyDay(uint32_t time) { return _localTimeServer.prettyDay(time); }
	String weatherConditions() { return _weatherServer.conditions(); }
	uint32_t currentTemp() { return _weatherServer.currentTemp(); }
	uint32_t highTemp() { return _weatherServer.highTemp(); }
	uint32_t lowTemp() { return _weatherServer.lowTemp(); }

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

	static void showDoneTick(Clock* self)
	{
		self->_stateMachine.sendInput(Input::ShowDone);
	}

	mil::StateMachine<State, Input> _stateMachine;
	mil::ButtonManager _buttonManager;
	mil::LocalTimeServer _localTimeServer;
	mil::WeatherServer _weatherServer;
	mil::BrightnessManager _brightnessManager;
	mil::Blinker _blinker;
	Ticker _secondTimer;
	Ticker _showDoneTimer;
	
	uint32_t _currentTime = 0;
	bool _needsUpdateTime = false;
	bool _needsUpdateWeather = false;
	bool _needsNetworkReset = false;
	bool _enteredConfigMode = false;
	
	struct tm  _settingTime;
	bool _settingTimeChanged = false;
	bool _enableNetwork = false;
	
	uint8_t _button = 1;
	String _configPortalName;
	
	
};

}
