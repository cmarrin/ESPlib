/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Application.h"

using namespace mil;

static constexpr int MaxHostnameLength = 40;
static const char* TAG = "Application";

Application::Application(WiFiPortal* portal, uint8_t led, const char* configPortalName)
    : _portal(portal)
    , _stateMachine({ { Input::LongPress, State::AskPreUserQuestion } })
    , _blinker(led, BlinkSampleRate)
    , _configPortalName(configPortalName)
{
}

void
Application::setup()
{
    _portal->begin(&_wfs);
	startStateMachine();
}

void
Application::loop()
{
	if (_needsNetworkReset) {
		startNetwork();
	}
    _portal->process();
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
    addParam("hostname", "Hostname", "esp32",  MaxHostnameLength);
    
    std::string hostname;
    if (getParamValue("hostname", hostname)) {
        _portal->setHostname(hostname.c_str());
    }
    
	if (_needsNetworkReset) {
		_needsNetworkReset = false;
		_portal->resetSettings();			
	}
	
	_portal->setConfigHandler([this](WiFiPortal* portal) {
		printf("Entered config mode:ip=%s, ssid='%s'\n", portal->localIP().c_str(), portal->getSSID());
		_blinker.setRate(ConfigRate);
		sendInput(Input::NetConfig, true);
		_enteredConfigMode = true;
	});

	if (!_portal->autoConnect(_configPortalName.c_str())) {
		printf("*** Failed to connect. Reset and try again\n");
        return;
	}
	
	if (_enteredConfigMode) {
		printf("*** Network didn't start up correctly. Reset and try again\n");
        return;
	}

	printf("Wifi connected, IP=%s\n", _portal->localIP().c_str());

	_enableNetwork = true;
	_blinker.setRate(ConnectedRate);
 
    // Setup the top level web page. If there is a /sys/shell.html in the filesystem
    // we add a button for it at the top of the landing page.
    if (!_wfs.begin(this, true)) {
        System::logE(TAG, "file system initialization failed");
    } else if (_wfs.exists("/sys/shell.html")) {
        setCustomMenuHTML("<form action='/fs/sys/shell.html' method='get'><button class='btn'>Shell</button></form><br/>");
    }
    
    _portal->startWebPortal();
	delay(500);
 
    if (!_wfs.begin(this, true)) {
        printf("***** file system initialization failed\n");
    }

    _portal->addStaticHTTPHandler("/fs", "/");
    
    _shell.begin();
    
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

