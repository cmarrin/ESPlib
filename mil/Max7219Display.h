//
//  ButtonManager.h
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
#include <Adafruit_GFX.h>
#include <Ticker.h>
#include <Max72xxPanel.h>
#include <time.h>

namespace mil {

	class Max7219Display
	{
	public:
		Max7219Display(std::function<void()> scrollDone);

		void clear();
		void setBrightness(uint32_t level);
		void showString(const String& string, uint32_t underscoreStart = 0, uint32_t underscoreLength = 0);
		void showTime(uint32_t currentTime, bool force = false);

	private:
		static constexpr uint32_t ScrollRate = 50;
		static constexpr uint32_t WatusiRate = 80;
		static constexpr int32_t WatusiMargin = 3;
		
		enum class ScrollType { Scroll, WatusiLeft, WatusiRight };

		void scrollString(const char* s, uint32_t scrollRate, ScrollType);
		void scroll();
		static void _scroll(Max7219Display* self) { self->scroll(); }
	
		// Look for control chars. \a means to use the compact font, \v means to scroll.
		// Returns offset into string past control chars
		uint32_t getControlChars(const String& s, bool& scroll);
	
		Max72xxPanel _matrix;
		const GFXfont* _currentFont = nullptr;
		Ticker _scrollTimer;
		String _scrollString;
		int32_t _scrollOffset;
		int16_t _scrollY;
		int32_t _scrollW;
		ScrollType _scrollType;
		std::function<void()> _scrollDone;
	};

}