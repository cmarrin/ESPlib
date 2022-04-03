/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "mil/Clock.h"

using namespace mil;

Clock::Clock(const String& startupMessage, const String& connectingMessage,
			 const String& timeCity, const String& weatherCity, 
			 uint8_t button, const String& configPortalName)
		: _stateMachine([this](const String s) { showString(s); }, { { Input::SelectLongPress, State::AskRestart } })
		, _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _localTimeServer(timeCity, [this]() { _needsUpdateTime = true; })
		, _weatherServer(weatherCity, [this]() { _needsUpdateWeather = true; })
		, _brightnessManager([this](uint32_t b) { handleBrightnessChange(b); }, LightSensor, 
							 InvertAmbientLightLevel, MinAmbientLightLevel, MaxAmbientLightLevel, NumberOfBrightnessLevels)
		, _blinker(LED_BUILTIN, BlinkSampleRate)
		, _startupMessage(startupMessage)
		, _connectingMessage(connectingMessage)
		, _button(button)
		, _configPortalName(configPortalName)
	{
		memset(&_settingTime, 0, sizeof(_settingTime));
		_settingTime.tm_mday = 1;
		_settingTime.tm_year = 100;
	}
	
void Clock::setup()
{
	mil::cout << "\n\n" << _startupMessage << "\n\n";
  
	_brightnessManager.start();

	_buttonManager.addButton(mil::Button(_button, _button, false, mil::Button::PinMode::Pullup));
	
	startStateMachine();

	_secondTimer.attach_ms(1000, secondTick, this);
}

void Clock::loop()
{
	if (_needsUpdateTime) {
		_needsUpdateTime = false;
		
		if (_enableNetwork) {
			if (_localTimeServer.update()) {
				_currentTime = _localTimeServer.currentTime();
				_stateMachine.sendInput(Input::Idle);
			} else {
				_stateMachine.sendInput(Input::UpdateFail);
			}
		}
	}
	if (_needsUpdateWeather) {
		_needsUpdateWeather = false;
		
		if (_enableNetwork) {
			if (_weatherServer.update()) {
				_stateMachine.sendInput(Input::Idle);
			} else {
				_stateMachine.sendInput(Input::UpdateFail);
			}
		}
	}
	if (_needsNetworkReset) {
		startNetwork();
	}
}


void Clock::startShowDoneTimer(uint32_t ms)
{
	_showDoneTimer.once_ms(ms, showDoneTick, this);
}
	
void Clock::startNetwork()
{
	_blinker.setRate(ConnectingRate);
	
	WiFiManager wifiManager;

	if (_needsNetworkReset) {
		_needsNetworkReset = false;
		wifiManager.resetSettings();			
	}
	
	wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
		mil::cout << F("Entered config mode:ip=") << 
					 WiFi.softAPIP() << F(", ssid='") << 
					 wifiManager->getConfigPortalSSID() << F("'\n");
		_blinker.setRate(ConfigRate);
		_stateMachine.sendInput(Input::NetConfig);
		_enteredConfigMode = true;
	});

	if (!wifiManager.autoConnect(_configPortalName.c_str())) {
		mil::cout << F("*** Failed to connect and hit timeout\n");
		ESP.reset();
		delay(1000);
	}
	
	if (_enteredConfigMode) {
		// If we've been in config mode, the network doesn't startup correctly, let's reboot
		ESP.reset();
		delay(1000);
	}

    WiFiMode_t currentMode = WiFi.getMode();
	mil::cout << F("Wifi connected, Mode=") << 
				 wifiManager.getModeString(currentMode) << 
				 F(", IP=") << 
				 WiFi.localIP() << mil::endl;

	_enableNetwork = true;
	_blinker.setRate(ConnectedRate);

	delay(500);
	_needsUpdateTime = true;
	_needsUpdateWeather = true;
	_stateMachine.sendInput(Input::Connected);
}
	
void Clock::startStateMachine()
{
	_stateMachine.addState(State::Connecting, _connectingMessage, [this] { startNetwork(); },
		{
			  { Input::Connected, State::Startup }
			, { Input::NetFail, State::NetFail }
			, { Input::NetConfig, State::NetConfig }
		}
	);
	_stateMachine.addState(State::NetConfig, [this] {
		String s = "\vConfigure WiFi. Connect to the '";
		s += _configPortalName;
		s += "' wifi network from your computer or mobile device, or press [select] to retry.";
		showString(s);
	},
		{
		      { Input::ShowDone, State::NetConfig }
		    , { Input::SelectClick, State::Connecting }
		    , { Input::Connected, State::Startup }
		    , { Input::NetFail, State::NetFail }
		}
	);
	_stateMachine.addState(State::NetFail, F("\vNetwork failed, press [select] to retry."),
		{
			  { Input::ShowDone, State::NetFail }
			, { Input::SelectClick, State::Connecting }
		}
	);
	_stateMachine.addState(State::UpdateFail, F("\vTime or weather update failed, press [select] to retry."),
		{
		      { Input::ShowDone, State::UpdateFail }
			, { Input::SelectClick, State::Connecting }
		}
	);
	_stateMachine.addState(State::Startup, [this] { showString(_startupMessage); },
		{
			  { Input::ShowDone, State::ShowTime }
			, { Input::SelectClick, State::ShowTime }
		}
	);
	_stateMachine.addState(State::ShowInfo, [this] { showInfo(); },
		{
			  { Input::ShowDone, State::ShowTime }
			, { Input::SelectClick, State::ShowTime }
		}
	);
	
	_stateMachine.addState(State::ShowTime, [this] { showTime(); }, State::Idle);
	
	_stateMachine.addState(State::Idle, [this] { showTime(); },
		{
			  { Input::SelectClick, State::ShowInfo }
			, { Input::Idle, State::Idle }
		}
	);
	
	// Restart
	_stateMachine.addState(State::AskRestart, F("\vRestart? (long press for yes)"),
		{
		  	  { Input::ShowDone, State::AskRestart }
			, { Input::SelectClick, State::AskResetNetwork }
			, { Input::SelectLongPress, State::Restart }
		}
	);
	_stateMachine.addState(State::Restart, [] { ESP.reset(); delay(1000); }, State::Connecting);
	
	// Network reset
	_stateMachine.addState(State::AskResetNetwork, F("\vReset network? (long press for yes)"),
		{
	  	  	  { Input::ShowDone, State::AskResetNetwork }
			, { Input::SelectClick, State::ShowTime }
			, { Input::SelectLongPress, State::VerifyResetNetwork }
		}
	);
	_stateMachine.addState(State::VerifyResetNetwork, F("\vAre you sure? (long press for yes)"),
		{
	  	  	  { Input::ShowDone, State::VerifyResetNetwork }
			, { Input::SelectClick, State::ShowTime }
			, { Input::SelectLongPress, State::ResetNetwork }
		}
	);
	_stateMachine.addState(State::ResetNetwork, [this] { _needsNetworkReset = true; }, State::NetConfig);
	
	// Start the state machine
	_stateMachine.gotoState(State::Connecting);
}
	
void Clock::handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event)
{
	if (button.id() == _button) {
		if (event == mil::ButtonManager::Event::Click) {
			_stateMachine.sendInput(Input::SelectClick);
		} else if (event == mil::ButtonManager::Event::LongPress) {
			_stateMachine.sendInput(Input::SelectLongPress);
		}
	}
}

void Clock::handleBrightnessChange(uint32_t brightness)
{
	setBrightness(brightness);
	mil::cout << "setting brightness to " << brightness << "\n";
}
