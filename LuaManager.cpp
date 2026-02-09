/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "LuaManager.h"

//#include "LuaLFS.h"
#include "WebFileSystem.h"

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
        if (_printQueue.size() >= MaxPrintQueueSize) {
            _status = Status::PrintBufferFull;
            _statusCond.wait(lk);
        }
        s += lua_tostring(L, i);
    }
    s += "\n";
    _printQueue.push_back(s);
}

std::string
LuaManager::getPrintStrings()
{
    // Give the command some time
    delay(500);
    
    std::unique_lock<std::mutex> lk(_mutex);
    
    if (_status == Status::NotStarted) {
        _statusCond.wait(lk);
    }

    std::string s;
    
    for (const auto& it : _printQueue) {
        s += it;
    }
    _printQueue.clear();
    
    if (_status == Status::PrintBufferFull) {
        _status = Status::Running;
        _statusCond.notify_all();
    }
    return s;
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
    // Tear it all down
    std::unique_lock<std::mutex> lk(_mutex);

    // Grab a pointer to the mgr so it sticks around until we return
    const auto& it = _managers.find(_id);
    if (it == _managers.end()) {
        // Uh oh. Manager is gone. For now just leave
        return;
    }
    
    std::shared_ptr<LuaManager> mgr = it->second;

    clearId(_id);
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
