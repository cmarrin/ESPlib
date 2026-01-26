/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "Shell.h"

#include "System.h"
#include "lua.hpp"

#include <sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

using namespace mil;

static const char* TAG = "Shell";
static constexpr int TelnetPort = 23;

bool
Shell::begin()
{
    _thread = std::thread([this]() { telnetServerTask(); });


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
Shell::telnetServerTask()
{
    int ipProtocol = 0;
    int keepAlive = 1;
    int keepIdle = 5;
    int keepInterval = 5;
    int keepCount = 3;
    int listenSocket;

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        System::logE(TAG, "Unable to create listen socket %i: errno %d", listenSocket, errno);
        return;
    }
    
    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    System::logI(TAG, "Listen socket %i created", listenSocket);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TelnetPort);

    int err = bind(listenSocket, (struct sockaddr *)&address, sizeof(address));
    if (err != 0) {
        System::logE(TAG, "Listen socket %i unable to bind: errno %d", listenSocket, errno);
        close(listenSocket);
        return;
    }
    System::logI(TAG, "Listen socket %i bound, port %d", listenSocket, TelnetPort);

    err = listen(listenSocket, 1);
    if (err != 0) {
        System::logE(TAG, "Error occurred during listen socket %i: errno %d", listenSocket, errno);
        close(listenSocket);
        return;
    }

    while (1) {
        System::logI(TAG, "Socket listening for new connection");
        struct sockaddr_storage sourceAddr;
        socklen_t addrLen = sizeof(sourceAddr);
        int sock = accept(listenSocket, (struct sockaddr *)&sourceAddr, &addrLen);
        
        if (sock < 0) {
            System::logE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        struct sockaddr_in *sin = (struct sockaddr_in *)&sourceAddr;
        unsigned char* ip = (unsigned char *)&sin->sin_addr.s_addr;
        System::logI(TAG, "Accepted connection:ip=%d.%d.%d.%d, socket=%d", ip[0], ip[1], ip[2], ip[3], sock);

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 5, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 5, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 3, &keepCount, sizeof(int));
    }  
}
