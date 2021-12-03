//
//  BrightnessManager.h
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

#pragma once

#include <mil.h>
#include <Ticker.h>

// BrightnessManager
//
// Periodically checks lightSensor port for a voltage and computes a brightness
// from that, with hysteresis. Samples at sampleRate ms and accumulates numSamples.
// Samples are clamped to maxLevels and then normalized to between 0 and numBrightness - 1.

namespace mil {

	class BrightnessManager
	{
	public:
		BrightnessManager(std::function<void(uint32_t brightness)> handler, 
						  uint8_t lightSensor, bool invert, uint32_t minLevel, uint32_t maxLevel, 
						  uint32_t numBrightness, int32_t minBrightness = -1, int32_t maxBrightness = -1, uint8_t numSamples = 5);
						  
		void start(uint32_t sampleRate = 100);
			
	private:
		static void compute(BrightnessManager* self) { self->computeBrightness(); }

		void computeBrightness();

		int32_t _currentAmbientLightLevel = std::numeric_limits<int32_t>::max();
		uint32_t _ambientLightAccumulator = 0;
		uint8_t _ambientLightSampleCount = 0;
		Ticker _ticker;
		uint8_t _lightSensor;
		bool _invert;
		uint32_t _minLevel;
		uint32_t _maxLevel;
		uint32_t _minBrightness;
		uint32_t _maxBrightness;
		uint32_t _numBrightness;
		uint8_t _numSamples;
		std::function<void(uint8_t brightness)> _handler;
	};
}
