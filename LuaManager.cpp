/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "LuaManager.h"

#include "LuaWFS.h"
#include "System.h"
#include "WebFileSystem.h"

#include <cstring>

using namespace mil;

std::map<uint8_t, std::shared_ptr<LuaManager>> LuaManager::_managers;
std::bitset<LuaManager::MaxIds> LuaManager::_usedIds;
std::mutex LuaManager::_mutex;

void
LuaManager::printHandler(lua_State *L)
{
    std::unique_lock<std::mutex> lk(_mutex);

    // Lua print is equivalent to println in other languages, so add a '\n'
    int nargs = lua_gettop(L);
    std::string s;
    
    for (int i = 1; i <= nargs; i++) {
        const char* nextString = lua_tostring(L, i);
        if (nextString) {
            size_t nextStringSize = strlen(nextString);

            // If an incoming string is longer than PrintBufferSize, ignore it
            if (nextStringSize >= PrintBufferSize) {
                continue;
            }
        
            size_t currentSize = strlen(_printBuffer);
            
            if (currentSize + nextStringSize >= PrintBufferSize) {
                // Mark buffer full and wait
                _status = Status::PrintBufferFull;
                _statusCond.wait(lk);
            }
            
            strcat(_printBuffer, nextString);
        }
    }
}

LuaManager::Status
LuaManager::getPrintStrings(std::string& s)
{
    std::unique_lock<std::mutex> lk(_mutex);
    
    s = _printBuffer;
    _printBuffer[0] = '\0';
    
    if (_status == Status::PrintBufferFull) {
        _status = Status::Running;
        _statusCond.notify_all();
    }
    return _status;
}

extern "C" {
    int printLua(lua_State *L)
    {
        // Get the LuaManager ptr
        lua_getglobal(L, "__LuaManager__");
        LuaManager* self = (LuaManager*) lua_touserdata(L, -1);
        lua_pop(L, 1);
        self->printHandler(L);
        return 0;
    }
}

LuaManager::LuaManager()
{
    _id = nextId();
    
    _luaState = luaL_newstate();
    luaL_openlibs(_luaState);
    luaL_requiref(_luaState, "wfs", luaopen_wfs, 1);
    
    // Set this as a lua global
    lua_pushlightuserdata(_luaState, this);
    lua_setglobal(_luaState, "__LuaManager__");
    
    // add a print method
    lua_register(_luaState, "__print__", printLua);
}

LuaManager::~LuaManager()
{
    clearId(_id);
    lua_close(_luaState);
}

// Access the LEDs from Lua
static int luaInitLED(lua_State* L)
{
    lua_Number numLEDs = lua_tonumber(L, -1);
    lua_Number pin = lua_tonumber(L, -2);
    lua_Number channel = lua_tonumber(L, -3);
    
    System::initLED(channel, pin, numLEDs);
    return 0;
}

static int luaSetLED(lua_State* L)
{
    lua_Number b = lua_tonumber(L, -1);
    lua_Number g = lua_tonumber(L, -2);
    lua_Number r = lua_tonumber(L, -3);
    lua_Number index = lua_tonumber(L, -4);
    lua_Number channel = lua_tonumber(L, -5);
    
    System::setLED(channel, index, r, g, b);
    return 0;
}

static int luaRefreshLEDs(lua_State* L)
{
    lua_Number channel = lua_tonumber(L, -1);
    
    System::refreshLEDs(channel);
    return 0;
}

static int luaDelay(lua_State* L)
{
    lua_Number ms = lua_tonumber(L, -1);
    
    delay(ms);
    return 0;
}

std::shared_ptr<LuaManager>
LuaManager::execute(const std::string& filename, int cpl, std::vector<std::string> args)
{
    std::shared_ptr<LuaManager> mgr = std::make_shared<LuaManager>();
    std::unique_lock<std::mutex> lk(_mutex);
    
    mgr->_command = filename;

    // Set the incoming cpl as a global
    lua_pushinteger(mgr->_luaState, cpl);
    lua_setglobal(mgr->_luaState, "__cpl__");
    
    // Set the require path
    std::string realRequirePath = WebFileSystem::realPath("/sys/?.lua");
    luaL_dostring(mgr->_luaState, (std::string("package.path = \"") + realRequirePath + "\"").c_str());

    // Set the LED and delay functions
    lua_pushcfunction(mgr->_luaState, luaInitLED);
    lua_setglobal(mgr->_luaState, "initLED");
    lua_pushcfunction(mgr->_luaState, luaSetLED);
    lua_setglobal(mgr->_luaState, "setLED");
    lua_pushcfunction(mgr->_luaState, luaRefreshLEDs);
    lua_setglobal(mgr->_luaState, "refreshLEDs");
    lua_pushcfunction(mgr->_luaState, luaDelay);
    lua_setglobal(mgr->_luaState, "delay");

    // Set an 'arg' global with the args
    lua_createtable(mgr->_luaState, int(args.size()), 0);
    int i = 1;
    
    for (const auto& it : args) {
        lua_pushnumber(mgr->_luaState, i);
        lua_pushstring(mgr->_luaState, it.c_str());
        lua_settable(mgr->_luaState, -3);
        ++i;
    }

    lua_setglobal(mgr->_luaState, "arg");

    _managers.emplace(mgr->_id, mgr);
    mgr->_thread = std::thread([mgr, filename]() { mgr->commandThread(filename); });

    // Wait until the thread gets going
    if (mgr->_status == Status::NotStarted) {
        mgr->_statusCond.wait(lk);
    }
    
    return mgr;
}

std::shared_ptr<LuaManager>
LuaManager::getManager(uint8_t id)
{
    const auto& it = _managers.find(id);
    if (it == _managers.end()) {
        // Uh oh. Manager is gone. For now just leave
        return nullptr;
    }
    
    // Grab a pointer to the mgr so it sticks around until we return
    return it->second;
}

void
LuaManager::finish()
{
    // Wait for the thread to exit
    _thread.join();
    
    // Tear it all down
    std::unique_lock<std::mutex> lk(_mutex);

    const auto& it = _managers.find(_id);
    if (it == _managers.end()) {
        // Uh oh. Manager is gone. For now just leave
        return;
    }
    
    // Grab a pointer to the mgr so it sticks around until we return
    std::shared_ptr<LuaManager> mgr = it->second;
    _managers.erase(it);
}

void
LuaManager::commandThread(const std::string& filename)
{
    {
        std::unique_lock<std::mutex> lk(_mutex);
        _status = Status::Running;
    
        // Notify main thread in case it's waiting for things to get going
        _statusCond.notify_all();
    }

    std::string realPath = WebFileSystem::realPath(filename);
    int result = luaL_dofile(_luaState, realPath.c_str());

    std::unique_lock<std::mutex> lk(_mutex);
    _status = Status::Done;
    _statusCond.notify_all();
    _result = result;
}
