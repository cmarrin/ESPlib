/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "mil/Clock.h"

Clock::Clock(const mil::ROMString& startupMessage)
		: _stateMachine([this](const String s) { showString(s); }, { { Input::SelectLongPress, State::AskRestart } })
		, _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _localTimeServer(TimeAPIKey, TimeCity, [this]() { _needsUpdateTime = true; })
		, _weatherServer(WeatherAPIKey, WeatherCity, [this]() { _needsUpdateWeather = true; })
		, _brightnessManager([this](uint32_t b) { handleBrightnessChange(b); }, LightSensor, 
							 InvertAmbientLightLevel, MinAmbientLightLevel, MaxAmbientLightLevel, NumberOfBrightnessLevels)
		, _blinker(BUILTIN_LED, BlinkSampleRate)
		, _startupMessage(startupMessage)
	{
		memset(&_settingTime, 0, sizeof(_settingTime));
		_settingTime.tm_mday = 1;
		_settingTime.tm_year = 100;
	}
	
void Clock::setup()
{
	Serial.begin(115200);
	delay(500);

	mil::cout << "\n\n" << _startupMessage << "\n\n";
  
	_brightnessManager.start();

	_buttonManager.addButton(mil::Button(SelectButton, SelectButton, false, mil::Button::PinMode::Pullup));
	
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
	
void Clock::startNetwork()
{
	_blinker.setRate(ConnectingRate);
	
	WiFiManager wifiManager;

	if (_needsNetworkReset) {
		_needsNetworkReset = false;
		wifiManager.resetSettings();			
	}
	
	wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
		mil::cout << L_F("Entered config mode:ip=") << 
					 WiFi.softAPIP() << L_F(", ssid='") << 
					 wifiManager->getConfigPortalSSID() << L_F("'\n");
		_blinker.setRate(ConfigRate);
		_stateMachine.sendInput(Input::NetConfig);
		_enteredConfigMode = true;
	});

	if (!wifiManager.autoConnect(ConfigPortalName)) {
		mil::cout << L_F("*** Failed to connect and hit timeout\n");
		ESP.reset();
		delay(1000);
	}
	
	if (_enteredConfigMode) {
		// If we've been in config mode, the network doesn't startup correctly, let's reboot
		ESP.reset();
		delay(1000);
	}

    WiFiMode_t currentMode = WiFi.getMode();
	mil::cout << L_F("Wifi connected, Mode=") << 
				 wifiManager.getModeString(currentMode) << 
				 L_F(", IP=") << 
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
	_stateMachine.addState(State::Connecting, L_F("\aConnecting..."), [this] { startNetwork(); },
		{
			  { Input::ShowDone, State::Connecting }
			, { Input::Connected, State::Startup }
			, { Input::NetFail, State::NetFail }
			, { Input::NetConfig, State::NetConfig }
		}
	);
	_stateMachine.addState(State::NetConfig, [this] {
		String s = "\vConfigure WiFi. Connect to the '";
		s += ConfigPortalName;
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
	_stateMachine.addState(State::NetFail, L_F("\vNetwork failed, press [select] to retry."),
		{
			  { Input::ShowDone, State::NetFail }
			, { Input::SelectClick, State::Connecting }
		}
	);
	_stateMachine.addState(State::UpdateFail, L_F("\vTime or weather update failed, press [select] to retry."),
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
	
	_stateMachine.addState(State::ShowTime, [this] { showTime(true); }, State::Idle);
	
	_stateMachine.addState(State::Idle, [this] { showTime(_currentTime); },
		{
			  { Input::SelectClick, State::ShowInfo }
			, { Input::Idle, State::Idle }
		}
	);
	
	// Restart
	_stateMachine.addState(State::AskRestart, L_F("\vRestart? (long press for yes)"),
		{
		  	  { Input::ShowDone, State::AskRestart }
			, { Input::SelectClick, State::AskResetNetwork }
			, { Input::SelectLongPress, State::Restart }
		}
	);
	_stateMachine.addState(State::Restart, [] { ESP.reset(); delay(1000); }, State::Connecting);
	
	// Network reset
	_stateMachine.addState(State::AskResetNetwork, L_F("\vReset network? (long press for yes)"),
		{
	  	  	  { Input::ShowDone, State::AskResetNetwork }
			, { Input::SelectClick, State::ShowTime }
			, { Input::SelectLongPress, State::VerifyResetNetwork }
		}
	);
	_stateMachine.addState(State::VerifyResetNetwork, L_F("\vAre you sure? (long press for yes)"),
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
	switch(button.id()) {
		case SelectButton:
		if (event == mil::ButtonManager::Event::Click) {
			_stateMachine.sendInput(Input::SelectClick);
		} else if (event == mil::ButtonManager::Event::LongPress) {
			_stateMachine.sendInput(Input::SelectLongPress);
		}
		break;
	}
}

void Clock::handleBrightnessChange(uint32_t brightness)
{
	setBrightness(brightness);
	mil::cout << "setting brightness to " << brightness << "\n";
}