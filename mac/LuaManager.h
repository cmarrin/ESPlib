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

class Lua
{
public:
    Lua();
    ~Lua();

private:
    lua_State _luaState;
};
