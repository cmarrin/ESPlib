/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Lua.h"

using namespace mil;

Lua::Lua()
{
    _luaState = luaL_newstate();
    luaL_openlibs(luaState);
}

Lua::~Lua()
{
    lua_close(_luaState);
}
