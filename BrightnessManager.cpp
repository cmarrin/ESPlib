//
//  BrightnessManager.cpp
//
//  Created by Chris Marrin on 3/25/2018
//
//

/*
Copyright (c) 2009-2018 Chris Marrin (chris@marrin.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, this 
      list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice, 
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution.

    - Neither the name of Marrinator nor the names of its contributors may be 
      used to endorse or promote products derived from this software without 
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE.
*/

#include "BrightnessManager.h"

//#define DEBUG_BRIGHTNESS

using namespace mil;

BrightnessManager::BrightnessManager(std::function<void(uint32_t brightness)> handler, uint8_t lightSensor, bool invert,
									 uint32_t minLevel, uint32_t maxLevel,
									 uint32_t numBrightness, int32_t minBrightness, int32_t maxBrightness,
									 uint8_t numSamples)
	: _lightSensor(lightSensor)
	, _invert(invert) 
	, _minLevel(minLevel)
	, _maxLevel(maxLevel)
	, _numBrightness(numBrightness)
	, _minBrightness((minBrightness < 0) ? 0 : minBrightness)
	, _maxBrightness((maxBrightness < 0) ? numBrightness : maxBrightness)
	, _numSamples(numSamples)
	, _handler(handler)
{
}

void BrightnessManager::start(uint32_t sampleRate)
{
	_ticker.attach_ms(sampleRate, [this]() { computeBrightness(); });
}

void BrightnessManager::computeBrightness()
{
	uint32_t ambientLightLevel = analogRead(_lightSensor);
#ifdef DEBUG_BRIGHTNESS
	cout << "**** Raw ambientLightLevel=" << ambientLightLevel << "\n";
#endif

	if (_invert) {
		ambientLightLevel = 1024 - ambientLightLevel;
	}

	ambientLightLevel = std::min(_maxLevel, std::max(_minLevel, ambientLightLevel));
	ambientLightLevel -= _minLevel;

	_ambientLightAccumulator += ambientLightLevel;

	if (++_ambientLightSampleCount >= _numSamples) {
		int32_t averageAmbientLightLevel = _ambientLightAccumulator / _numSamples;
		_ambientLightAccumulator = 0;
		_ambientLightSampleCount = 0;

#ifdef DEBUG_BRIGHTNESS
		cout << "**** Average ambientLightLevel=" << averageAmbientLightLevel << "\n";
#endif

		// Use hysteresis to avoid throbbing the light level.
		int32_t diff = _maxLevel - _minLevel;
		int32_t ambientLightStepSize = diff / Hysteresis;
		int32_t currentAmbientLightLevel = static_cast<int32_t>(_currentAmbientLightLevel);

		if (averageAmbientLightLevel <= currentAmbientLightLevel + ambientLightStepSize &&
			averageAmbientLightLevel >= currentAmbientLightLevel - ambientLightStepSize) {
			return;
		}

		_currentAmbientLightLevel = averageAmbientLightLevel;
		uint32_t brightnessLevel = (_currentAmbientLightLevel * _numBrightness + diff / 2) / diff;
		if (brightnessLevel < _minBrightness) {
			brightnessLevel = _minBrightness;
		} else if (brightnessLevel > _maxBrightness) {
			brightnessLevel = _maxBrightness;
		}

#ifdef DEBUG_BRIGHTNESS
		cout << "**** Sending brightnessLevel=" << brightnessLevel << "\n";
#endif

		_handler(brightnessLevel);
	}
}
