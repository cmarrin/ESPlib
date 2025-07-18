/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Application.h"

using namespace mil;

#ifdef ARDUINO
#if defined(ESP8266)
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif
#else
WiFiClass WiFi;
#endif

static constexpr int MaxHostnameLength = 40;

Application::Application(uint8_t led, const char* configPortalName)
    : _stateMachine({ { Input::LongPress, State::AskPreUserQuestion } })
    , _blinker(led, BlinkSampleRate)
    , _configPortalName(configPortalName)
{
    addParam("hostname", "Hostname", "set_new_hostname",  MaxHostnameLength);
}

void
Application::initParams()
{
    for (auto& it : _params) {
        const char* id = it->getID();
        String savedValue = _prefs.getString(id);
        if (savedValue.length() == 0) {
            const char* value = it->getValue();
            _prefs.putString(id, value);
            cout << F("No '") << id << F("' saved. Setting it to default: '") << value << F("'\n");
        } else {
            it->setValue(savedValue.c_str(), it->getValueLength());
            cout << F("Setting '") << id << F("' to saved value: '") << savedValue.c_str() << F("'\n");
        }
    } 
}
	
void
Application::setup()
{
    _prefs.begin("ESPLib");
    
    initParams();
	startStateMachine();
}

void
Application::loop()
{
	if (_needsNetworkReset) {
		startNetwork();
	}
#if defined(ESP8266)
    MDNS.update();
#endif
    _wifiManager.process();
}

void
Application::startShowDoneTimer(uint32_t ms)
{
	_showDoneTimer.once_ms(ms, [this]() { sendInput(Input::ShowDone, true); });
}

void
Application::startNetwork()
{
	_blinker.setRate(ConnectingRate);
	
    std::vector<const char *> menu = { "custom", "wifi", "info", "restart", "sep", "update" };
    _wifiManager.setMenu(menu);
    
    for (auto& it : _params) {
        _wifiManager.addParameter(it.get());
    }
    
    _wifiManager.setHostname(getParamValue("hostname"));
	_wifiManager.setDebugOutput(true);
	_wifiManager.setDarkMode(true);
    _wifiManager.setShowInfoErase(true);
    
	if (_needsNetworkReset) {
		_needsNetworkReset = false;
		_wifiManager.resetSettings();			
	}
	
	_wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
		cout << F("Entered config mode:ip=") << 
					 WiFi.softAPIP() << F(", ssid='") << 
					 _wifiManager.getConfigPortalSSID() << F("'\n");
		_blinker.setRate(ConfigRate);
		sendInput(Input::NetConfig, true);
		_enteredConfigMode = true;
	});

	if (!_wifiManager.autoConnect(_configPortalName.c_str())) {
		cout << F("*** Failed to connect and hit timeout\n");
		restart();
		delay(1000);
	}
	
	if (_enteredConfigMode) {
		// If we've been in config mode, the network doesn't startup correctly, let's reboot
        saveParams();
		restart();
		delay(1000);
	}

	cout << F("Wifi connected, IP=") << WiFi.localIP() << mil::endl;

	_enableNetwork = true;
	_blinker.setRate(ConnectedRate);
 
    _wifiManager.setSaveParamsCallback([this]()
    {
        saveParams();
        delay(2000);
        restart();
    });

    _wifiManager.startWebPortal();

    if (!MDNS.begin(getParamValue("hostname")))  {             
        cout << F("***** Error starting mDNS\n");
    } else {
        cout << F("mDNS started, hostname=") << getParamValue("hostname") << "\n";
    }

	delay(500);
	sendInput(Input::Connected, false);
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
	
	// PreUserQuestion
	_stateMachine.addState(State::AskPreUserQuestion, [this]
    {
        if (_havePreUserQuestion) {
            showString(Message::AskPreUserQuestion);
        } else {
        	_stateMachine.gotoState(State::AskRestart);
        }
    },
		{
		  	  { Input::ShowDone, State::AskPreUserQuestion }
			, { Input::Click, State::AskRestart }
			, { Input::LongPress, State::AnswerPreUserQuestion }
		}
	);
	_stateMachine.addState(State::AnswerPreUserQuestion, [this] { preUserAnswer(); }, State::ForceShowMain);
	
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
	
	// PostUserQuestion
	_stateMachine.addState(State::AskPostUserQuestion, [this]
    {
        if (_havePostUserQuestion) {
            showString(Message::AskPostUserQuestion);
        } else {
        	_stateMachine.gotoState(State::ForceShowMain);
        }
    },
		{
		  	  { Input::ShowDone, State::AskPostUserQuestion }
			, { Input::Click, State::ForceShowMain }
			, { Input::LongPress, State::AnswerPostUserQuestion }
		}
	);
	_stateMachine.addState(State::AnswerPostUserQuestion, [this] { postUserAnswer(); }, State::ForceShowMain);
	
	// Start the state machine
	_stateMachine.gotoState(State::Connecting);
}

