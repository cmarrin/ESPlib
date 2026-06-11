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

static const char* TAG = "LuaManager";

std::map<uint8_t, std::shared_ptr<LuaManager>> LuaManager::_managers;
std::bitset<LuaManager::MaxIds> LuaManager::_usedIds;
std::mutex LuaManager::_mutex;

void
LuaManager::printHandler(lua_State *L)
{
    std::unique_lock<std::mutex> lk(_mutex);

    int nargs = lua_gettop(L);
    
    for (int i = 1; i <= nargs; i++) {
        const char* s = lua_tostring(L, i);
        if (s) {
            size_t size = strlen(s);
            print(_printCB, s, size);
        }
    }
}

LuaManager::LuaManager(PrintCB printCB)
    : _printCB(printCB)
{
    _id = nextId();
    
    System::logI(TAG, "creating manager with id=%d\n", int(_id));
    
    _luaState = luaL_newstate();
    luaL_openlibs(_luaState);
    luaL_requiref(_luaState, "wfs", luaopen_wfs, 1);
    
    // Set this as a lua global
    lua_pushlightuserdata(_luaState, this);
    lua_setglobal(_luaState, "__LuaManager__");
}

LuaManager::~LuaManager()
{
    System::logI(TAG, "deleting manager with id=%d\n", int(_id));
}

static int printLua(lua_State *L)
{
    // Get the LuaManager ptr
    lua_getglobal(L, "__LuaManager__");
    LuaManager* self = (LuaManager*) lua_touserdata(L, -1);
    lua_pop(L, 1);
    self->printHandler(L);
    return 0;
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

static int luaSetLEDs(lua_State* L)
{
    lua_Number b = lua_tonumber(L, -1);
    lua_Number g = lua_tonumber(L, -2);
    lua_Number r = lua_tonumber(L, -3);
    lua_Number count = lua_tonumber(L, -4);
    lua_Number startIndex = lua_tonumber(L, -5);
    lua_Number channel = lua_tonumber(L, -6);
    
    System::setLEDs(channel, startIndex, count, r, g, b);
    return 0;
}

static int luaHSVToRGB(lua_State* L)
{
    lua_Number v = std::max(0, std::min(255, int(lua_tonumber(L, -1))));
    lua_Number s = std::max(0, std::min(255, int(lua_tonumber(L, -2))));
    lua_Number h = std::max(0, std::min(255, int(lua_tonumber(L, -3))));

    uint8_t r, g, b;
    Graphics::hsvToRGB(r, g, b, h * 256, s, v);
    
    lua_pushnumber(L, r);
    lua_pushnumber(L, g);
    lua_pushnumber(L, b);
    return 3;
}

static int luaRGBToHSV(lua_State* L)
{
    lua_Number b = std::max(0, std::min(255, int(lua_tonumber(L, -1))));
    lua_Number g = std::max(0, std::min(255, int(lua_tonumber(L, -2))));
    lua_Number r = std::max(0, std::min(255, int(lua_tonumber(L, -3))));

    uint8_t h, s, v;
    Graphics::rgbToHSV(h, s, v, r, g, b);
    
    lua_pushnumber(L, h);
    lua_pushnumber(L, s);
    lua_pushnumber(L, v);
    return 3;
}

static int luaHTMLToHSV(lua_State* L)
{
    const char* str = lua_tostring(L, -1);

    uint8_t h, s, v;
    Graphics::htmlToHSV(h, s, v, str);
    
    lua_pushnumber(L, h);
    lua_pushnumber(L, s);
    lua_pushnumber(L, v);
    return 3;
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
    
    System::delay(ms);
    return 0;
}

static int luaMillis(lua_State* L)
{
    lua_pushnumber(L, System::millis());
    return 1;
}

static int luaGetEvent(lua_State* L)
{
    // Lua expects a single value which is either a table or nil

    // Get the LuaManager ptr
    lua_getglobal(L, "__LuaManager__");
    LuaManager* self = (LuaManager*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    LuaManager::Event ev;
    if (LuaManager::getEvent(self->id(), ev)) {
        lua_createtable(L, 0, 1);
        lua_pushstring(L, ev.second.c_str());
        lua_setfield(L, -2, ev.first.c_str());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int8_t
LuaManager::execute(const std::string& filename, std::vector<std::string> args,
                    int cpl, std::function<void(const char*, size_t)> printCB)
{
    std::shared_ptr<LuaManager> mgr = std::make_shared<LuaManager>(printCB);
    
    mgr->_command = filename;

    // Set the incoming cpl as a global
    lua_pushinteger(mgr->_luaState, cpl);
    lua_setglobal(mgr->_luaState, "__cpl__");
    
    // Set the require path
    std::string realRequirePath = WebFileSystem::realPath("/sys/?.lua");
    luaL_dostring(mgr->_luaState, (std::string("package.path = \"") + realRequirePath + "\"").c_str());
    
    // set the path for file i/o
    std::filesystem::current_path(WebFileSystem::realPath("/sys/"));

    // add a print method
    lua_register(mgr->_luaState, "__print__", printLua);

    // Set the LED and delay functions
    lua_pushcfunction(mgr->_luaState, luaInitLED);
    lua_setglobal(mgr->_luaState, "initLED");
    lua_pushcfunction(mgr->_luaState, luaSetLED);
    lua_setglobal(mgr->_luaState, "setLED");
    lua_pushcfunction(mgr->_luaState, luaSetLEDs);
    lua_setglobal(mgr->_luaState, "setLEDs");
    lua_pushcfunction(mgr->_luaState, luaHSVToRGB);
    lua_setglobal(mgr->_luaState, "hsvToRGB");
    lua_pushcfunction(mgr->_luaState, luaRGBToHSV);
    lua_setglobal(mgr->_luaState, "rgbToHSV");
    lua_pushcfunction(mgr->_luaState, luaHTMLToHSV);
    lua_setglobal(mgr->_luaState, "htmlToHSV");
    lua_pushcfunction(mgr->_luaState, luaRefreshLEDs);
    lua_setglobal(mgr->_luaState, "refreshLEDs");
    lua_pushcfunction(mgr->_luaState, luaDelay);
    lua_setglobal(mgr->_luaState, "delay");
    lua_pushcfunction(mgr->_luaState, luaMillis);
    lua_setglobal(mgr->_luaState, "millis");
    lua_pushcfunction(mgr->_luaState, luaGetEvent);
    lua_setglobal(mgr->_luaState, "getEvent");

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
    mgr->_thread.detach();
    return mgr->_id;
}

void
LuaManager::terminate(int8_t id)
{
    std::unique_lock<std::mutex> lk(_mutex);

    std::shared_ptr<LuaManager> mgr = getManager(id);
    if (!mgr) {
        // Uh oh. Manager is gone. For now just leave
        System::logE(TAG, "termination failed, id %d does not exist", int(id));
        return;
    }
    
    System::logI(TAG, "terminating Lua program with id %d", int(id));
    lua_stop(mgr->_luaState);
}

std::shared_ptr<LuaManager>
LuaManager::getManager(uint8_t id)
{
    const auto& it = _managers.find(id);
    if (it == _managers.end()) {
        // Uh oh. Manager is gone. For now just leave
        return nullptr;
    }
    
    return it->second;
}

void
LuaManager::waitForFinish()
{
    std::unique_lock<std::mutex> lk(_mutex);
    if (_status != Status::Done) {
        _statusCond.wait(lk);
    }
}

void
LuaManager::commandThread(const std::string& filename)
{
    {
        std::unique_lock<std::mutex> lk(_mutex);
        _status = Status::Running;
    }

    std::string realPath = WebFileSystem::realPath(filename.c_str());
    if (luaL_dofile(_luaState, realPath.c_str()) != LUA_OK) {
        std::unique_lock<std::mutex> lk(_mutex);
        _status = Status::Done;
        _errorString = lua_tostring(_luaState, -1);
        std::string err = " Lua file '" + command() + "' failed to run: " + _errorString + "\n";
        print(_printCB, err.c_str(), err.length());
    } else {
        std::unique_lock<std::mutex> lk(_mutex);
        _status = Status::Done;
        _errorString = "";
    }
    
    _statusCond.notify_all();

    std::unique_lock<std::mutex> lk(_mutex);

    clearId(_id);
    lua_close(_luaState);

    const auto& it = _managers.find(_id);
    if (it != _managers.end()) {
        _managers.erase(it);
    }
}

void
LuaManager::sendEvent(int8_t id, const Event&ev)
{
    std::unique_lock<std::mutex> lk(_mutex);
    std::shared_ptr<LuaManager> mgr = getManager(id);
    if (mgr) {
        mgr->_eventQueue.push(ev);
    }
}

bool
LuaManager::getEvent(int8_t id, Event& ev)
{
    std::unique_lock<std::mutex> lk(_mutex);
    std::shared_ptr<LuaManager> mgr = getManager(id);
    if (mgr) {
        if (!mgr->_eventQueue.empty()) {
            ev = mgr->_eventQueue.front();
            mgr->_eventQueue.pop();
            return true;
        }
    }
    return false;
}
