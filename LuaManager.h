/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include "lua.hpp"

#include <map>
#include <thread>

// Lua Interface

namespace mil {

class LuaManager
{
public:
    using PrintHandlerCB = std::function<void(const char* s)>;
    
    LuaManager();
    ~LuaManager();
    
    static std::shared_ptr<LuaManager> execute(const std::string& filename);
    void finish();
    
    const char* toString(int idx) const { return lua_tostring(_luaState, idx); }
    
    void printHandler(lua_State *);
    
    std::string getPrintStrings();
    
    bool isDone() const
    {
        std::unique_lock<std::mutex> lk(_mutex);
        return _status == Status::Done;
    }
    
    int result() const
    {
        std::unique_lock<std::mutex> lk(_mutex);
        return _result;
    }
    
    uint8_t id() const { return _id; }
    
private:
    static constexpr int MaxPrintQueueSize = 5;
    static constexpr int MaxIds = 32;
    enum class Status { NotStarted, Running, PrintBufferFull, WaitingForInput, Done };
    
    void commandThread(const std::string& filename);
    
    static uint8_t nextId() {
        for (int i = 0; i < MaxIds; ++i) {
            if (!_usedIds[i]) {
                _usedIds.set(i);
                return i;
            }
        }
        return MaxIds;
    }
    
    static void clearId(uint8_t id) { _usedIds.reset(id); }
    
    lua_State* _luaState = nullptr;
    std::thread _thread;
    std::condition_variable _statusCond;
    
    std::vector<std::string> _printQueue;
    
    static std::map<uint8_t, std::shared_ptr<LuaManager>> _managers;
    static std::bitset<MaxIds> _usedIds;
    static std::mutex _mutex;
    
    uint8_t _id;
    Status _status = Status::NotStarted;
    int _result = 0;
};

}
