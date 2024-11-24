/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2021, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Clock.h"

#include "Application.h"

using namespace mil;

Clock::Clock(Application* app, const char* zipCode,
            uint8_t lightSensor, bool invertAmbientLightLevel, uint32_t minBrightness, uint32_t maxBrightness,
            uint8_t button, BrightnessChangeCB brightnessChangeCB)
        : _app(app)
		, _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _timeWeatherServer(zipCode, [this]() { _needsUpdate = true; })
		, _brightnessManager([this](uint32_t b) { handleBrightnessChange(b); }, lightSensor, 
							 invertAmbientLightLevel, minBrightness, maxBrightness, NumberOfBrightnessLevels)
        , _brightnessChangeCB(brightnessChangeCB)
		, _button(button)
	{
		memset(&_settingTime, 0, sizeof(_settingTime));
		_settingTime.tm_mday = 1;
		_settingTime.tm_year = 100;
	}
	
void Clock::setup()
{
	_brightnessManager.start();

	_buttonManager.addButton(mil::Button(_button, _button, false, mil::Button::PinMode::Pullup));
	
	_secondTimer.attach_ms(1000, [this]() {
        _currentTime++;
        _app->sendInput(Input::Idle);
    });
}

void Clock::loop()
{
	if (_needsUpdate) {
		_needsUpdate = false;
		
		if (_app && _app->isNetworkEnabled()) {
			if (_timeWeatherServer.update()) {
				_currentTime = _timeWeatherServer.currentTime();
				_app->sendInput(Input::Idle);
			} else {
				_app->sendInput(Input::UpdateFail);
			}
		}
	}
}

void
Clock::handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event)
{
	if (button.id() == _button) {
		if (event == mil::ButtonManager::Event::Click) {
			_app->sendInput(Input::Click);
		} else if (event == mil::ButtonManager::Event::LongPress) {
			_app->sendInput(Input::LongPress);
		}
	}
}

void
Clock::handleBrightnessChange(uint32_t brightness)
{
	_brightnessChangeCB((brightness < 5) ? 5 : brightness);
}
