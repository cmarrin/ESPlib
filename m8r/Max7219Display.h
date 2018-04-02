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

#include <Adafruit_GFX.h>
#include <Ticker.h>
#include <Max72xxPanel.h>
#include <time.h>

namespace m8r {

	class Max7219Display
	{
	public:
		enum class Font { Normal, Compact };
	
		Max7219Display();
	
		virtual void scrollDone() = 0;

		void clear();
		void setBrightness(float level);
		void setString(const String& string, Font = Font::Normal, bool colon = false, bool pm = false);
		void scrollString(const String& s, uint32_t scrollRate, Font = Font::Normal);
		void setTime(uint32_t currentTime, Font = Font::Normal);

	private:	
		void scroll();
	
		void setFont(Font);
	
	
		static void _scroll(Max7219Display* self) { self->scroll(); }
	
	private:
		Max72xxPanel _matrix;
		Ticker _scrollTimer;
		String _scrollString;
		int32_t _scrollOffset;
	};

}