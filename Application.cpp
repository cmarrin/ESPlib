/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Application.h"

using namespace mil;

#ifndef ARDUINO
WiFiClass WiFi;
#endif

Application::Application(uint8_t led, const char* configPortalName)
		: _stateMachine({ { Input::LongPress, State::AskRestart } })
		, _blinker(led, BlinkSampleRate)
		, _configPortalName(configPortalName)
	{ }
	
void
Application::setup()
{
	startStateMachine();
}

void
Application::loop()
{
	if (_needsNetworkReset) {
		startNetwork();
	}
}

void
Application::startShowDoneTimer(uint32_t ms)
{
	_showDoneTimer.once_ms(ms, [this]() { _stateMachine.sendInput(Input::ShowDone); });
}

void
Application::startNetwork()
{
	_blinker.setRate(ConnectingRate);
	
	WiFiManager wifiManager;

	wifiManager.setDebugOutput(false);

	if (_needsNetworkReset) {
		_needsNetworkReset = false;
		wifiManager.resetSettings();			
	}
	
	wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
		cout << F("Entered config mode:ip=") << 
					 WiFi.softAPIP() << F(", ssid='") << 
					 wifiManager->getConfigPortalSSID() << F("'\n");
		_blinker.setRate(ConfigRate);
		_stateMachine.sendInput(Input::NetConfig);
		_enteredConfigMode = true;
	});

	if (!wifiManager.autoConnect(_configPortalName.c_str())) {
		cout << F("*** Failed to connect and hit timeout\n");
		restart();
		delay(1000);
	}
	
	if (_enteredConfigMode) {
		// If we've been in config mode, the network doesn't startup correctly, let's reboot
		restart();
		delay(1000);
	}

	cout << F("Wifi connected, IP=") << F(", IP=") << WiFi.localIP() << mil::endl;

	_enableNetwork = true;
	_blinker.setRate(ConnectedRate);

	delay(500);
	_stateMachine.sendInput(Input::Connected);
}
	
void Application::startStateMachine()
{
	_stateMachine.addState(State::Connecting, [this] { showString(Message::Connecting); startNetwork(); },
		{
			  { Input::Connected, State::Startup }
			, { Input::NetFail, State::NetFail }
			, { Input::NetConfig, State::NetConfig }
		}
	);
	_stateMachine.addState(State::NetConfig, [this] { showString(Message::NetConfig); },
		{
		      { Input::ShowDone, State::NetConfig }
		    , { Input::Click, State::Connecting }
		    , { Input::Connected, State::Startup }
		    , { Input::NetFail, State::NetFail }
		}
	);
	_stateMachine.addState(State::NetFail, [this] { showString(Message::NetFail); },
		{
			  { Input::ShowDone, State::NetFail }
			, { Input::Click, State::Connecting }
		}
	);
	_stateMachine.addState(State::UpdateFail, [this] { showString(Message::UpdateFail); },
		{
		      { Input::ShowDone, State::UpdateFail }
			, { Input::Click, State::Connecting }
		}
	);
	_stateMachine.addState(State::Startup, [this] { showString(Message::Startup); },
		{
			  { Input::ShowDone, State::ForceShowMain }
			, { Input::Click, State::ForceShowMain }
		}
	);
	_stateMachine.addState(State::ShowSecondary, [this] { showSecondary(); },
		{
			  { Input::ShowDone, State::ForceShowMain }
			, { Input::Click, State::ForceShowMain }
		}
	);
	
	_stateMachine.addState(State::ForceShowMain, [this] { showMain(true); }, State::Idle);
	_stateMachine.addState(State::ShowMain, [this] { showMain(); }, State::Idle);
	
	_stateMachine.addState(State::Idle, [this] { showMain(); },
		{
			  { Input::Click, State::ShowSecondary }
			, { Input::Idle, State::Idle }
		}
	);
	
	// Restart
	_stateMachine.addState(State::AskRestart, [this] { showString(Message::AskRestart); },
		{
		  	  { Input::ShowDone, State::AskRestart }
			, { Input::Click, State::AskResetNetwork }
			, { Input::LongPress, State::Restart }
		}
	);
	_stateMachine.addState(State::Restart, [this] { restart(); delay(1000); }, State::Connecting);
	
	// Network reset
	_stateMachine.addState(State::AskResetNetwork, [this] { showString(Message::AskResetNetwork); },
		{
	  	  	  { Input::ShowDone, State::AskResetNetwork }
			, { Input::Click, State::ForceShowMain }
			, { Input::LongPress, State::VerifyResetNetwork }
		}
	);
	_stateMachine.addState(State::VerifyResetNetwork, [this] { showString(Message::VerifyResetNetwork); },
		{
	  	  	  { Input::ShowDone, State::VerifyResetNetwork }
			, { Input::Click, State::ForceShowMain }
			, { Input::LongPress, State::ResetNetwork }
		}
	);
	_stateMachine.addState(State::ResetNetwork, [this] { _needsNetworkReset = true; }, State::NetConfig);
	
	// Start the state machine
	_stateMachine.gotoState(State::Connecting);
}

