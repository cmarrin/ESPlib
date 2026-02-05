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
        LuaManager lua;
    
        if (lua.execute(path.c_str()) != LUA_OK) {
            printf("%s\n", lua.toString(-1));
            std::string err = "Lua file '" + path + "' failed to run: " + lua.toString(-1) + "\n";
            p->sendHTTPResponse(404, "text/plain", err.c_str());
        } else {
            System::logI(TAG, "***** Ran Lua command '%s'\n", path.c_str());

            // Send the string as plain text, but prepend it with a one character id.
            // If it is 0x20 (space) then this is the last response. Values from 0x21
            // to 0x7f are the id of the Lua command being executed. ID values start
            // at 0, so the value sent is 0x21 + id. This allows for 95 simultaneous
            // commands, which should be more than enough.
            //
            // FIXME: for now we only support 0x20 as the id.
            std::string response = " " + lua.getAllPrintStrings();
            p->sendHTTPResponse(200, "text/plain", response.c_str());
        }
    }
}
