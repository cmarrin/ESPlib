/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include <string>
#include <thread>

// Cross platform shell server. Uses sockets to open port 23 and when
// connected runs a shell based on linenoise. Executes lua programs
// from the file system, along with some built-in commands

namespace mil {

class Shell
{
  public:    
    bool begin();
    
  private:
    void serverTask();
    void clientTask(int sock);
    
    std::thread _thread;
};

}
