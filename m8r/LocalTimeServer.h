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

#include <m8r.h>
#include <JsonListener.h>
#include <Ticker.h>
#include <ctime>

// LocalTimeServer
//
// Get the local time feed from timezonedb.com. Parses the current time 
// and date

namespace m8r {

	class LocalTimeServer
	{
	public:
		class MyJsonListener : public JsonListener
		{
		public:
			virtual ~MyJsonListener() { }
	
			virtual void key(String key) override;
			virtual void value(String value) override;
			virtual void whitespace(char c) override { }
			virtual void startDocument() override { }
			virtual void endArray() override { }
			virtual void endObject() override { }
			virtual void endDocument() override { }
			virtual void startArray() override { }
			virtual void startObject() override { }
	
			uint32_t localEpoch() const { return _localEpoch; }
			int32_t localTZOffset() const { return _localTZOffset; }
	
		private:
			enum class State {
				None, LocalEpoch, LocalTZOffset
			};
		
			State _state = State::None;
		
			uint32_t _localEpoch = 0;
			int32_t _localTZOffset = 0;
		};

		LocalTimeServer(const String& key, const String& city, std::function<void()> handler)
			: _key(key)
			, _city(city)
			, _handler(handler)
		{
		}
		
		uint32_t currentTime() const { return _currentTime; }
		
		static String strftime(const char* format, uint32_t time);
		static String strftime(const char* format, const struct tm&);
		static String prettyDay(uint32_t time);
	
		bool update();
		
	private:
		static void fire(LocalTimeServer* self) { self->_handler(); }

		String _key;
		String _city;
		Ticker _ticker;
		
		uint32_t _currentTime = 0;
		
		std::function<void()> _handler;
	};

}