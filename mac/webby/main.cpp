/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "MacWebServer.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

int main(int argc, char* argv[])
{
    int port = (argc > 1) ? std::stoi(argv[1]) : 3000;
    if (port == 0) {
        port = 3000;
    }
    
    mil::Server server;
    server.start(port);
    
    while (true) {
        usleep(1000);
    }

    return 0;
}
