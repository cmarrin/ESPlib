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

#include "LuaManager.h"
#include "libtelnet.h"

// Cross platform shell server. Uses sockets to open port 23 and when
// connected runs a shell based on linenoise. Executes lua programs
// from the file system, along with some built-in commands

namespace mil {

class Application;
class WiFiPortal;

class Shell
{
  public:
    ~Shell();
    
    bool begin(Application* app);
    
    int8_t handleShellCommand(const std::string& cmd, PrintCB);
    void terminateShellCommand(int8_t id) { LuaManager::terminate(id); }

  private:
    void tcpServerTask();
    void tcpClientTask(int sock);
    void telnetHandler(int sock, telnet_t* telnet, telnet_event_t* event);
    
    static void _telnetHandler(telnet_t* telnet, telnet_event_t* ev, void* userData)
    {
        TelnetClient* client = reinterpret_cast<TelnetClient*>(userData);
        client->_shell->telnetHandler(client->_sock, telnet, ev);
    }
        
    std::string makeAbsolutePath(const std::string& path) const;
    
    std::string cwd() const { return _dirs.empty() ? "/" : _dirs.back(); }
    
    void showDirs(PrintCB) const;
    
    Application* _app = nullptr;
    
    std::vector<std::string> _dirs;
    
    std::thread _serverThread;
    int _serverSocket = -1;
    bool _terminating = false;
    
    struct TelnetClient
    {
        TelnetClient(int sock, Shell* shell) : _sock(sock), _shell(shell) { }
        int _sock;
        Shell* _shell;
    };
    
    int _termChars = 80;
    int _termLines = 24;
};

}
