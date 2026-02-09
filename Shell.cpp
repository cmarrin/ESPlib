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
    if (p->hasHTTPArg("cmd")) {
        std::string cmd;
        cmd = HTTPParser::urlDecode(p->getHTTPArg("cmd"));
        
        // FIXME: for now ignore the args
        if (cmd.empty()) {
            p->sendHTTPResponse(400, "text/plain", "no command supplied");
            return;
        }

        std::vector<std::string>args = HTTPParser::split(cmd, ' ');
        cmd = args.front();
        args.erase(args.begin());
        
        // FIXME: for now all commands are .lua in the sys folder
        std::string path = makeCmdPath("/sys/", cmd.c_str(), ".lua");
        
        if (!WebFileSystem::exists(path.c_str())) {
            p->sendHTTPResponse(404, "text/plain", (path + ": command not found").c_str());
            return;
        }

        // Execute command
        std::shared_ptr<LuaManager> lua = LuaManager::execute(path.c_str());
        
        // Get any returned print strings. This not only gets the strings
        // but the does the proper waits and state changes
        std::string printString = lua->getPrintStrings();
        
        // Once we have a response we are either done or the command has
        // more to do. In the former case we send an idChar of ' ' which
        // causes the Lua command line to terminate and show a prompt.
        // Otherwise we send an idChar indicating the id of the running
        // command. This causes the Shell to make another request to get
        // more output. We continue this until we're done.
        char idChar = lua->isDone() ? ' ' : 0x20 + lua->id() + 1;
       
        // Command is either still executing because buffer is full, or its finished
        // Either way, check for errors then get any strings and put them in the
        // response.
        if (lua->isDone() && lua->result() != LUA_OK) {
            printf("%s\n", lua->toString(-1));
            std::string err = "Lua file '" + path + "' failed to run: " + lua->toString(-1) + "\n";
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
            std::string response = std::string(1, idChar)  + printString;
            p->sendHTTPResponse(200, "text/plain", response.c_str());
            
            if (lua->isDone()) {
                lua->finish();
                System::logI(TAG, "***** Ran Lua command '%s'\n", path.c_str());
            }
        }
    }
}
