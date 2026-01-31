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
static constexpr int TelnetPort = 23;

bool
Shell::begin(Application* app)
{
    app->addHTTPHandler("/shell", WiFiPortal::HTTPMethod::Get, [this](WiFiPortal* p) { handleShellCommand(p); });

//    // Test running a script
//    lua_State* L = luaL_newstate();
//	luaL_openlibs(L);
//
//	std::string sample = "print(\"Hello World from Lua inside c++!\")";
// 
// 	int err = luaL_loadbuffer(L, sample.c_str(), sample.length(), "mysample");
//	if (err) {
//		printf("Error initializing lua with hello world script: %i, %s\n", err, lua_tostring(L, -1));
//		lua_close(L);
//		return(0);
//	}
//
//	err = lua_pcall(L, 0, 0, 0);
//	if (err) {
//		printf("Error running lua hello world script: %i, %s\n", err, lua_tostring(L, -1));
//		lua_close(L);
//		return(0);
//	}
//
//	printf("Success running hello world script\n");
//	lua_close(L);

    return true;
}

void
Shell::handleShellCommand(WiFiPortal* p)
{
    printf("******* handleShellCommand: enter\n");
    if (p->hasHTTPArg("cmd")) {
        std::string cmd = HTTPParser::urlDecode(p->getHTTPArg("cmd"));
        printf("******* handleShellCommand: cmd='%s'\n", cmd.c_str());

        // FIXME: for now expect command to be a single word. Eventually we need to split out args
        if (cmd.empty()) {
            p->sendHTTPResponse(400, "application/json", "{\"status\":\"error\",\"message\":\"Command not provided\"}");
            return;
        }
        
        // FIXME: for now all commands are .lua in the sys folder
        std::string path("/sys/");
        path += cmd + "lua";
        if (!WebFileSystem::exists(path.c_str())) {
            p->sendHTTPResponse(404, "application/json", "{\"status\":\"error\",\"message\":\"Command not found\"}");
            return;
        }
        printf("******* handleShellCommand: before execute");

        // Execute command
        LuaManager lua;
    
        if (lua.execute(WebFileSystem::realPath(path).c_str()) != LUA_OK) {
            printf("%s\n", lua.toString(-1));
            std::string err = "Lua file '" + path + "' failed to run: " + lua.toString(-1) + "\n";
            p->sendHTTPResponse(404, "text/plain", err.c_str());
        } else {
            printf("***** Running Lua file '%s'\n", path.c_str());
            p->sendHTTPResponse(200, "text/html", "<center><h1>Lua Runtime</h1><h2>No output</h2></center>");
        }
        printf("******* handleShellCommand: after execute");
    }
}
