//
//  MenuSystem.cpp
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

#include "MenuSystem.h"

using namespace mil;

MenuSystem::MenuSystem(std::function<void(const MenuItem*)> showHandler, bool wrap)
	: _wrap(wrap)
	, _showHandler(showHandler)
{
}

void MenuItem::show(MenuSystem* menuSystem)
{
	menuSystem->_showHandler(this);
}

bool Menu::move(Move dir, bool wrap, Action action)
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
    return false;
}
