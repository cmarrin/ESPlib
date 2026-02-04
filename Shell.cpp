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
            p->sendHTTPResponse(200, "text/plain", lua.getAllPrintStrings().c_str());
        }
    }
}
