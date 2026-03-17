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

#include <sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

using namespace mil;

static const char* TAG = "Shell";
static constexpr int TelnetPort = 23;

bool
Shell::begin(Application* app)
{
    _app = app;
    
    // Set cwd
    _dirs.clear();
    std::string cwd;
    if (!_app->getNVSParam("cwd", cwd)) {
        // Not found, set cwd to '/'
        cwd = "/";
        _app->setNVSParam("cwd", cwd);
    }
    
    _dirs.push_back(cwd);
    
    _serverThread = std::thread([this]() { tcpServerTask(); });

    return true;
}

std::string
Shell::makeAbsolutePath(const std::string& path) const
{
    if (path.empty()) {
        return cwd();
    }
    if (path[0] == '/') {
        return path;
    }
    if (path.length() >= 2 && path[0] == '.' && path[1] == '/') {
        // Relative path
        return cwd() + path.substr(1);
    }
    return std::filesystem::path(cwd() + "/" + path).lexically_normal().string();
}

// This is hackery. When trying to make a path in handleHTTPRequest the suffix would always get dropped
// on espidf. moving the concat to a separate method fixes it. Until I get to the bottom of it, this
// solution will do.
static std::string makeCmdPath(const char* root, const char* cmd, const char* suffix)
{
    std::string s = std::string(root) + cmd + suffix;
    return s;
}

// There are several built-in commands (adapted from bash) but since we
// don't current support most of bash's features, this is a small
// subset:
//
//      cd          - Change to passed dir
//      date        - Shows the current date
//      dirs        - Shows the dirs stack
//      history     - Show history stack (implemented in shell.html)
//      popd        - Pop the top of the dirs stack and go to the next entry in the stack
//      pushd       - Go to the passed dire and push it onto the dirs stack
//      pwd         - Show the current dir
//

// cd, dirs, popd, pushd and pwd all have to do with maintaining the current working
// directory (cwd). When cd is executed the cwd is set to that and saved as an NVS
// param. There is also a stack of directories. When 'pushd' is executed the cwd
// is changed and saved like in 'cd' but it is also pushed onto the top of the
// stack. When popd is called the top of the dirs stack is popped and the next
// entry is made the cwd. The dirs stack is not preserved as an NVS param so at each
// reboot it is reset to the cwd.

void
Shell::showDirs(PrintCB printCB) const
{
    std::string dirs;
    for (int i = int(_dirs.size() - 1); i >= 0; --i) {
        dirs += _dirs[i] + " ";
    };
    printCB((dirs + "\n").c_str(), dirs.length() + 1);
}

void
Shell::handleShellCommand(const std::string& incomingCmd, PrintCB printCB)
{
    std::string cmd = incomingCmd;
    if (cmd.back() == '\n') {
        cmd.pop_back();
    }
    if (cmd.back() == '\r') {
        cmd.pop_back();
    }
    if (cmd.empty()) {
        std::string s = "no command supplied";
        printCB(s.c_str(), s.length());
        return;
    }

    // Make sure the WebFileSystem has the right cwd
    WebFileSystem::setCWD(_dirs.back().c_str());

    std::vector<std::string>args = HTTPParser::split(cmd, ' ');
    cmd = args.front();
    args.erase(args.begin());
    
    // See if it's a built-in command
    // FIXME: For now just do an if cascade. Maybe we use a map at some time?
    if (cmd == "date") {
        std::string date(" ");
        date += _app->clock() ? _app->clock()->prettyTime().c_str() : "no clock";
        date += "\n";
        printCB(date.c_str(), date.length());
        return;
    }
    
    if (cmd == "pwd") {
        printCB((_dirs.back() + "\n").c_str(), _dirs.back().length() + 1);
        return;
    }
    
    if (cmd == "cd" || cmd == "pushd") {
        // cd pops the top of the dirs stack and replaces it
        // pushd just pushes the dir onto the top of the stack
        std::string cwd = makeAbsolutePath(args.empty() ? "/" : args[0]);

        if (!WebFileSystem::exists(cwd.c_str())) {
            std::string response = cmd + ": " + cwd + ": No such file or directory\n";
            printCB(response.c_str(), response.length());
        } else {
            if (cmd == "cd") {
                _dirs.pop_back();
            }
            _dirs.push_back(cwd);
            WebFileSystem::setCWD(cwd.c_str());
            if (cmd != "cd") {
                showDirs(printCB);
            }
        }
        return;
    }
    
    if (cmd == "popd") {
        if (_dirs.size() == 1) {
            std::string s = "popd: directory stack empty\n";
            printCB(s.c_str(), s.length());
        } else {
            _dirs.pop_back();
            WebFileSystem::setCWD(_dirs.back().c_str());
            showDirs(printCB);
        }
        return;
    }
    
    if (cmd == "dirs") {
        showDirs(printCB);
        return;
    }
    
    // FIXME: for now all commands are .lua in the sys folder
    std::string path = makeCmdPath("/sys/", cmd.c_str(), ".lua");
    
    if (!WebFileSystem::exists(path.c_str())) {
        std::string s = path + ": command not found";
        printCB(s.c_str(), s.length());
        return;
    }

    // Execute command
    LuaManager::execute(path.c_str(), _termChars, args, printCB);
}

void
Shell::tcpServerTask()
{
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
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
        int keepAlive = 1;
        int keepIdle = 5;
        int keepInterval = 5;
        int keepCount = 3;

        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 5, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 5, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 3, &keepCount, sizeof(int));
        
        // Start a thread with a shell for the client
        std::thread clientThread = std::thread([this, sock]() { tcpClientTask(sock); });
        clientThread.detach();
        
        // FIXME: ultimately we need to keep all the client threads in a list
        // along with their socket so we can kill them and stuff like that
        
        delay(20);
    }  
}

static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_NAWS, TELNET_WONT,   TELNET_DO},
    { TELNET_TELOPT_ECHO, TELNET_WONT,   TELNET_DO},
    { TELNET_TELOPT_NEW_ENVIRON, TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_TTYPE,		TELNET_WILL, TELNET_DONT },
    { -1, 0, 0 }
};

void
Shell::telnetHandler(int sock, telnet_t *telnet, telnet_event_t *event)
{
    if (event->type == TELNET_EV_WILL && event->iac.cmd == TELNET_TELOPT_NAWS) {
        telnet_iac(telnet, TELNET_TELOPT_NAWS);
        return;
    }
    if (event->type == TELNET_EV_SUBNEGOTIATION && event->sub.telopt == TELNET_TELOPT_NAWS) {
        // Should have a buffer with 4 bytes. First two are width (msb first), second is height
        // Ignore if not 2 bytes and set to a default
        if (event->sub.size != 4) {
            _termChars = 80;
            _termLines = 24;
        } else {
            _termChars = (int(uint8_t(event->sub.buffer[0])) << 8) | uint8_t(event->sub.buffer[1]);
            _termLines = (int(uint8_t(event->sub.buffer[2])) << 8) | uint8_t(event->sub.buffer[3]);
        }
        return;
    }
    if (event->type == TELNET_EV_DATA) {
        handleShellCommand(std::string(event->data.buffer, event->data.size), [telnet](const char* buf, size_t size)
        {
            telnet_send(telnet, buf, size);
        });
        return;
    }
    if (event->type == TELNET_EV_SEND) {
        ssize_t result = send(sock, event->data.buffer, event->data.size, 0);
        if (result < 0) {
            System::logE(TAG, "Error sending data:%s", strerror(errno));
        }
        return;
    }
}

void
Shell::tcpClientTask(int sock)
{
    // Create the telnet handler
    TelnetClient client(sock, this);
    telnet_t* telnet = telnet_init(telopts, _telnetHandler, 0, &client);
    if (!telnet) {
        System::logE(TAG, "Failed to initialize libtelnet=%s", strerror(errno));
        return;
    }
        
    // FIXME: Allocate the buffer (and probably bigger) to avoid blowing the stack
    char buf[128];
    while (true) {
        ssize_t result = recv(sock, buf, 127, 0);
        if (result == 0) {
            System::logI(TAG, "Connection closed");
            break;
        }
        if (result < 0) {
            System::logE(TAG, "Error receiving data, result=%d", int(result));
            break;
        }
        
        telnet_recv(telnet, buf, result);
    }
    telnet_free(telnet);
}
