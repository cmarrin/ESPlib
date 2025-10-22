/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include "lua.hpp"

// Lua Interface

namespace mil {

class LuaManager
{
public:
    LuaManager();
    ~LuaManager();
    
    int execute(const std::string& filename);
    
    const char* toString(int idx) const { return lua_tostring(_luaState, idx); }
    
private:
    lua_State* _luaState = nullptr;
};

}
