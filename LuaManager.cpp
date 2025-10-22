/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "LuaManager.h"

using namespace mil;

LuaManager::LuaManager()
{
    _luaState = luaL_newstate();
    luaL_openlibs(_luaState);
}

LuaManager::~LuaManager()
{
    lua_close(_luaState);
}

int
LuaManager::execute(const std::string& filename)
{
    return luaL_dofile(_luaState, filename.c_str());
}
