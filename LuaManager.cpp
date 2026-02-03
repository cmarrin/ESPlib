/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "LuaManager.h"

using namespace mil;

void
LuaManager::printHandler(lua_State *L)
{
    int nargs = lua_gettop(L);
    
    for (int i = 1; i <= nargs; i++) {
        const char* s = lua_tostring(L, i);
        if (s) {
            _printQueue.push(s);
        }
    }
}

extern "C" {
    int printLua(lua_State *L)
    {
        // Get the LuaManager ptr
        lua_getglobal(L, "__LuaManager__");
        LuaManager* self = (LuaManager*) lua_touserdata(L, -1);
        self->printHandler(L);
        return 0;
    }
}

LuaManager::LuaManager()
{
   _luaState = luaL_newstate();
    luaL_openlibs(_luaState);
    
    // Set this as a lua global
    lua_pushlightuserdata(_luaState, this);
    lua_setglobal(_luaState, "__LuaManager__");
    
    // Override the print method
    lua_pushcfunction(_luaState, printLua);
    lua_setglobal(_luaState, "print");
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
