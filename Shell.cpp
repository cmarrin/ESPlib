/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Shell.h"

#include "Application.h"
#include "HTTPParser.h"
#include "System.h"
#include "WebFileSystem.h"
#include "LuaManager.h"

using namespace mil;

static const char* TAG = "Shell";

bool
Shell::begin(Application* app)
{
    app->addHTTPHandler("/shell", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { handleShellCommand(p); });
    return true;
}

// This is hackery. When trying to make a path in handleShellCommand the suffix would always get dropped
// on espidf. moving the concat to a separate method fixes it. Until I get to the bottom of it, this
// solution will do.
static std::string makeCmdPath(const char* root, const char* cmd, const char* suffix)
{
    std::string s = std::string(root) + cmd + suffix;
    return s;
}

void
Shell::handleShellCommand(WiFiPortal* p)
{
    std::shared_ptr<LuaManager> lua;
    
    if (p->hasHTTPArg("cmd")) {
        std::string cmd = HTTPParser::urlDecode(p->getHTTPArg("cmd"));
        int cpl = std::stoi(HTTPParser::urlDecode(p->getHTTPArg("cpl")));
        
        // Failsafe
        if (cpl <= 0 || cpl >= 10000) {
            cpl = 80;
        }
        
        // FIXME: for now ignore the args
        if (cmd.empty()) {
            p->sendHTTPResponse(400, "text/plain", " no command supplied");
            return;
        }

        std::vector<std::string>args = HTTPParser::split(cmd, ' ');
        cmd = args.front();
        args.erase(args.begin());
        
        // FIXME: for now all commands are .lua in the sys folder
        std::string path = makeCmdPath("/sys/", cmd.c_str(), ".lua");
        
        if (!WebFileSystem::exists(path.c_str())) {
            p->sendHTTPResponse(404, "text/plain", (" " + path + ": command not found").c_str());
            return;
        }

        // Execute command
        lua = LuaManager::execute(path.c_str(), cpl);
        
        // Let the command run a bit
        delay(200);
    } else if (p->hasHTTPArg("id")) {
        std::string idChar;
        idChar = HTTPParser::urlDecode(p->getHTTPArg("id"));
        uint8_t id = idChar[0] - 0x21;
        
        lua = LuaManager::getManager(id);
    }
    
    // Get any returned print strings. This not only gets the strings
    // but the does the proper waits and state changes
    std::string printString;
    LuaManager::Status status = lua ? lua->getPrintStrings(printString) : LuaManager::Status::Done;

    // Command is either still executing because buffer is full, or its finished
    // Either way, check for errors then get any strings and put them in the
    // response.
    if (status == LuaManager::Status::Done && lua && lua->result() != LUA_OK) {
        std::string err = " Lua file '" + lua->command() + "' failed to run: " + lua->toString(-1) + "\n";
        p->sendHTTPResponse(404, "text/plain", err.c_str());
    } else {
        // We are either done or not. Either way, get any strings that have been
        // buffered and send them in the response.
        //
        // Send the string as plain text, but prepend it with a one character id.
        // If it is 0x20 (space) then this is the last response (isDone == true).
        // Values from 0x21 to 0x7f are the id of the Lua command being executed.
        // Id values start at 0, so the value sent is 0x21 + id. This allows for
        // 95 simultaneous commands, which should be more than enough.
        char idChar = ' ';
        if (lua) {
            idChar = lua->isDone() ? ' ' : 0x20 + lua->id() + 1;
        }
        
        std::string response = std::string(1, idChar)  + printString;

        p->sendHTTPResponse(200, "text/plain", response.c_str());
        
        if (status == LuaManager::Status::Done) {
            if (lua) {
                lua->finish();
                System::logI(TAG, "***** Ran Lua command '%s'\n", lua->command().c_str());
            }
        }
    }
}
