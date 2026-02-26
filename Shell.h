/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include <string>
#include <thread>
#include <vector>

// Cross platform shell server. Uses sockets to open port 23 and when
// connected runs a shell based on linenoise. Executes lua programs
// from the file system, along with some built-in commands

namespace mil {

class Application;
class WiFiPortal;

class Shell
{
  public:    
    bool begin(Application* app);
    
  private:
    void handleShellCommand(WiFiPortal*);
    
    std::string makeAbsolutePath(const std::string& path) const;
    
    std::string cwd() const { return _dirs.empty() ? "/" : _dirs.back(); }
    
    Application* _app = nullptr;
    
    std::vector<std::string> _dirs;
};

}
