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

bool
Shell::begin(Application* app)
{
    app->addHTTPHandler("/shell", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { handleShellCommand(p); });
    return true;
}

void
Shell::handleShellCommand(WiFiPortal* p)
{
    if (p->hasHTTPArg("cmd")) {
        std::string cmd = HTTPParser::urlDecode(p->getHTTPArg("cmd"));

        // FIXME: for now ignore the args
        if (cmd.empty()) {
            p->sendHTTPResponse(400, "text/plain", "no command supplied");
            return;
        }

        std::vector<std::string>args = HTTPParser::split(cmd, ' ');
        cmd = args.front();
        args.erase(args.begin());
        
        // FIXME: for now all commands are .lua in the sys folder
        std::string path("/sys/");
        path += cmd + "lua";
        if (!WebFileSystem::exists(path.c_str())) {
            p->sendHTTPResponse(404, "text/plain", (cmd + ": command not found").c_str());
            return;
        }

        // Execute command
        LuaManager lua;
    
        if (lua.execute(WebFileSystem::realPath(path).c_str()) != LUA_OK) {
            printf("%s\n", lua.toString(-1));
            std::string err = "Lua file '" + path + "' failed to run: " + lua.toString(-1) + "\n";
            p->sendHTTPResponse(404, "text/plain", err.c_str());
        } else {
            printf("***** Running Lua file '%s'\n", path.c_str());
            p->sendHTTPResponse(200, "text/plain", "...done");
        }
    }
}
