/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// Application is the main class for ESP based projects. It sets up the 
// WiFi network. On the first time through or when network settings 
// have been reset, it uses WiFiManager to setup a captive network to 
// let the user set the ssid and password for the network. Then it 
// restarts and connects to the local network. On subsequent runs it
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

#include <cassert>
#include <ctime>

#include "mil.h"
#include "StateMachine.h"
#include "Blinker.h"

#ifdef ARDUINO
#include <Preferences.h>
#include <WiFiManager.h>
#endif

namespace mil {

// All rates in ms

// rate light blinks in ms in each mode
static constexpr uint32_t ConnectingRate = 500;
static constexpr uint32_t ConfigRate = 200;
static constexpr uint32_t ConnectedRate = 2000;
static constexpr uint32_t BlinkSampleRate = 1;

enum class State {
	Connecting, NetConfig, NetFail, UpdateFail, 
	Startup, Idle, ShowMain, ForceShowMain, ShowSecondary,
    AskPreUserQuestion, AnswerPreUserQuestion,
	AskRestart, Restart,
	AskResetNetwork, VerifyResetNetwork, ResetNetwork,
    AskPostUserQuestion, AnswerPostUserQuestion,
};

enum class Input { Idle, Click, LongPress, ShowDone, Connected, NetConfig, NetFail, UpdateFail };

enum class Message { Startup, Connecting, NetConfig, NetFail, UpdateFail,
                     AskPreUserQuestion, AskRestart, AskResetNetwork, VerifyResetNetwork, AskPostUserQuestion
                   };

class Application
{
public:
	Application(uint8_t led, const char* hostname, const char* configPortalName);
	
	virtual void setup();
	virtual void loop();
	
	virtual void showString(Message) = 0;
	virtual void showMain(bool force = false) = 0;
	virtual void showSecondary() = 0;
 
    virtual void preUserAnswer() { }
    virtual void postUserAnswer() { }
 
    void setHaveUserQuestions(bool pre, bool post) { _havePreUserQuestion = pre; _havePostUserQuestion = post; }

    void sendInput(Input input, bool inCallback)
    {
        bool prevInCallback = _inCallback;
        _inCallback = inCallback;
        _stateMachine.sendInput(input);
        _inCallback = prevInCallback;
    }
    
    bool isInCallback() const { return _inCallback; }
    
    bool isNetworkEnabled() const { return _enableNetwork; }

	void startShowDoneTimer(uint32_t ms);

    void restart()
    {
#ifdef ARDUINO
        ESP.restart();
#else
        cout << "***** RESTART *****\n";
#endif
    }
    
    const char* zipCode() const { return _zipCode.getValue(); }
    
private:
	void startNetwork();
	void startStateMachine();
 
	mil::StateMachine<State, Input> _stateMachine;
	mil::Blinker _blinker;
	Ticker _showDoneTimer;
	
	bool _needsNetworkReset = false;
	bool _enteredConfigMode = false;
	
	bool _enableNetwork = false;
	
	CPString _configPortalName;

    WiFiManager wifiManager;
    WiFiManagerParameter _zipCode;
    WiFiManagerParameter _hostname;
    Preferences prefs;

    bool _inCallback = false;
    
    bool _havePreUserQuestion = false;
    bool _havePostUserQuestion = false;
};

}
