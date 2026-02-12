/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "LuaManager.h"

//#include "LuaLFS.h"
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
    strcat(_printBuffer, "\n");
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
    //luaopen_lfs  (_luaState);
    
    // Set this as a lua global
    lua_pushlightuserdata(_luaState, this);
    lua_setglobal(_luaState, "__LuaManager__");
    
    // Override the print method
    lua_pushcfunction(_luaState, printLua);
    lua_setglobal(_luaState, "print");
    
    // Set the cwd
//    std::string chdir("lfs.chdir(\"");
//    chdir += WebFileSystem::realPath("");
//    chdir += "\")";
//    luaL_dostring(_luaState, chdir.c_str());
}

LuaManager::~LuaManager()
{
    clearId(_id);
    lua_close(_luaState);
}

std::shared_ptr<LuaManager>
LuaManager::execute(const std::string& filename)
{
    std::shared_ptr<LuaManager> mgr = std::make_shared<LuaManager>();
    std::unique_lock<std::mutex> lk(_mutex);

    _managers.emplace(mgr->_id, mgr);
    mgr->_thread = std::thread([mgr, filename]() { mgr->commandThread(filename); });

    // Wait until the thread gets going
    if (mgr->_status == Status::NotStarted) {
        mgr->_statusCond.wait(lk);
    }
    
    return mgr;
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
