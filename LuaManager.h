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
    using PrintHandlerCB = std::function<void(const char* s)>;
    
    LuaManager();
    ~LuaManager();
    
    int execute(const std::string& filename);
    
    const char* toString(int idx) const { return lua_tostring(_luaState, idx); }
    
    void printHandler(lua_State *);
    
    bool getNextPrintString(std::string& s, bool append = false)
    {
        if (_printQueue.empty()) {
            return false;
        }
        
        if (append) {
            s += _printQueue.front();
        } else {
            s = _printQueue.front();
        }
        _printQueue.pop();
        return true;
    }
    
    std::string getAllPrintStrings()
    {
        std::string s;
        while (getNextPrintString(s, true)) ;
        return s;
    }
    
private:
    lua_State* _luaState = nullptr;

    std::queue<std::string> _printQueue;
};

}
