/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// File System based on LittleFS. It can be integrated with WebServer
// so HTTP requests can be made to upload files, read existing
// files, create directories, delete files and directories and do
// directory listings.

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
	Application(uint8_t led, const char* configPortalName);
	
	virtual void setup();
	virtual void loop();
	
	virtual void showString(Message) { }
	virtual void showMain(bool force = false) { }
	virtual void showSecondary() { }
 
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
    
    void addParam(const char *id, const char *label, const char *defaultValue, int length)
    {
        _params.push_back(std::make_shared<WiFiManagerParameter>(id, label, defaultValue, length));
    }
    
    const char* getParamValue(const char* id) const
    {
        for (auto& it : _params) {
            if (strcmp(it->getID(), id) == 0) {
                return it->getValue();
            }
        }
        return nullptr;
    }
    
    void saveParams()
    {
        for (auto& it : _params) {
            _prefs.putString(it->getID(), it->getValue());
        }
    }

    void setTitle(const char* title) { _wifiManager.setTitle(title); }
    void setCustomMenuHTML(const char* s) { _wifiManager.setCustomMenuHTML(s); }
    String getHTTPArg(const char* name) { return _wifiManager.server->arg(name); }
    int getHTTPArgCount() const { return _wifiManager.server->args(); }
    void sendHTTPPage(const char* page) { _wifiManager.server->send(200, "text/html", page); }
    void addHTTPHandler(const char* page, std::function<void(void)> h) { _wifiManager.server->on(page, h); }
    void addCustomHTTPHandler(RequestHandler* h) { _wifiManager.server->addHandler(h); }
    const HTTPRaw& getHTTPRaw() { return _wifiManager.server->raw(); }
    
private:
	void startNetwork();
	void startStateMachine();
 
    void initParams();
    
	mil::StateMachine<State, Input> _stateMachine;
	mil::Blinker _blinker;
	Ticker _showDoneTimer;
	
	bool _needsNetworkReset = false;
	bool _enteredConfigMode = false;
	
	bool _enableNetwork = false;
	
	String _configPortalName;

    WiFiManager _wifiManager;
    Preferences _prefs;
    std::vector<std::shared_ptr<WiFiManagerParameter>> _params;

    bool _inCallback = false;
    
    bool _havePreUserQuestion = false;
    bool _havePostUserQuestion = false;
};

}
