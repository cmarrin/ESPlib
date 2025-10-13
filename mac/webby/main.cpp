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

void handleClient(int client_socket_fd)
{
    char client_req_buffer[1024];
    
    read(client_socket_fd, client_req_buffer, 1024);
    mil::HttpRequest req;
    
    req.parseRequest(client_req_buffer);
    std::string mimetype = req.getMimeType(req.path());
    
    mil::HttpResponse res;
    
    std::string body = req.readHtmlFile(req.path());
    std::string response = res.frameHttpResponse("200", "OK", req.headers(), body, mimetype);

    write(client_socket_fd, response.c_str(), response.length());
    
    close(client_socket_fd);
}

int main(int argc, char* argv[])
{
    std::string port = (argc > 1) ? argv[1] : "3000";
    
    mil::Server server(port);
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int s_fd;
    s_fd = server.start();
    client_addr_size = sizeof(struct sockaddr_in); 

    while (1) {
        int client_socket_fd = accept(s_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_socket_fd < 0) {
            printf("Failed to accept client request.\n");
            exit(1);
        }
       
        // Create a new thread to handle the client
        std::thread clientThread(handleClient, client_socket_fd);
        clientThread.detach(); // Detach the thread to allow it to run independently
    }

    return 0;
}
