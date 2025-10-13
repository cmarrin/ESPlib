/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebShell.h"

#include "Application.h"
#include "lua.hpp"

using namespace mil;

bool
WebShell::begin(Application* app)
{
    app->addHTTPHandler("/shell", [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(404, "text/plain", "Not yet impolemented");
        return true;
    });

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
