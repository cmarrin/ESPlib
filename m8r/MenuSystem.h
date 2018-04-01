//
//  MenuSystem.h
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

#include <Arduino.h>
#include <memory>
#include <vector>

namespace m8r {

	// MenuSystem 
	//
	// A set of classes to implement a menu system. Designed for constained displays
	// with an interface of one or more buttons. Create a subclas of MenuSystem with
	// implementations of the render and one or more of the button methods. Then
	// build the menu system out of Menu and MenuItem instances
	
	class MenuSystem;

	class MenuItem
	{
		friend class MenuSystem;
		
	public:
		enum class Move { Up, Down, In, Out };

		MenuItem(const String& s) : _string(s) { }
		
		enum class Action { None, Activate };
		
		virtual bool move(Move dir, bool wrap, Action action = Action::None)
		{
		}

		virtual void menuItemEvent(const MenuItem&) { }
		virtual void show(MenuSystem* menuSystem);
		
		void start(MenuSystem* menuSystem)
		{
			_active = true;
			show(menuSystem);
		}
		
		const String& string() const { return _string; }
				
	protected:
		String _string;
		bool _active = false;
	};
	
	class Menu : public MenuItem
	{
	public:
		Menu(const String& s) : MenuItem(s) { }
		
		void addMenuItem(std::shared_ptr<MenuItem>&);
		
	private:		
		virtual bool move(Move dir, bool wrap, Action action = Action::None) override
		{
			if (action == Action::Activate) {
				_active = true;
				_cur = 0;
				return true;
			}
			
			if (!_active) {
				if (move(dir, wrap)) {
					return true;
				}
				
				// We've just be activated by an Out
				_active = true;
				return true;
			}
			
			switch(dir) {
				case Move::Up:
				if (_cur == 0 && !wrap) {
					return true;
				}
				_cur = _menuItems.size() - 1;
				return true;

				case Move::Down:
				if (_cur == _menuItems.size() - 1 && !wrap) {
					return true;
				}
				_cur += 1;
				return true;

				case Move::In:
				_active = false;
				return move(dir, wrap, Action::Activate);

				case Move::Out:
				_active = false;
				return false;
			}
		}
		
		std::vector<std::shared_ptr<MenuItem>> _menuItems;
		int16_t _cur = -1;
	};
	
	class MenuSystem
	{
	public:
		MenuSystem(bool wrap = true) : _wrap(wrap) { }
		
		void setMenu(std::shared_ptr<MenuItem>& item) { _menu = item; }
		
		void start() { _menu->start(this); }
		void move(MenuItem::Move dir) { _menu->move(dir, _wrap); }
		
		virtual void showMenuItem(const MenuItem*) = 0;
		
	private:
		std::shared_ptr<MenuItem> _menu;
		bool _wrap;
	};
	
}
