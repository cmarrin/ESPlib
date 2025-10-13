#include <cstring>
#include <thread>
#include <iostream>
#include<map>
#include<fstream>
#include<sstream>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>

class HttpRequest{
public:
  std::string method;
  std:: string path;
  std::map<std::string, std::string> headers;
  void parseRequest(const std::string& rawRequest);
  
  std::string readHtmlFile(const std::string &path);
  std::string getMimeType(const std::string &path);
};

void
HttpRequest::parseRequest(const std::string& rawRequest)
{
    int currindex=0;
    while(currindex < rawRequest.length()) {
        if(rawRequest[currindex] == ' ') {
            break;
        }
        method += rawRequest[currindex];
        currindex++;
    }
    headers["method"] = method;

    currindex++;
    while(currindex < rawRequest.length()){
        if(rawRequest[currindex] == ' '){
            break;
        }
        path += rawRequest[currindex];
        currindex++;
    }
   
    headers["path"] = path;
}
std::string
HttpRequest::readHtmlFile(const std::string &path)
{
    std::string filename = path.substr(1,path.length());
    if (filename.empty()) {
        filename = "index.html";
    }
   
    std::ifstream file(filename); //ifstream used for reading file if exists.
    int flag = 0;
    if (!file) {
        flag=1;
        std::cerr<<"File not found."<<std::endl;
    }

    if (flag == 0) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    return "";
}

std::string
HttpRequest::getMimeType(const std::string &path)
{
    std::map<std::string, std::string> mimeTypes = {
        {"html","text/html"},
        {"css","text/css"},
        {"js","text/javascript"},
        {"jpg","image/jpeg"},
        {"png","image/png"}
    };
    
    std::string fileExtension = path.substr(path.find_last_of(".") + 1);
    return mimeTypes[fileExtension];
}

class HttpResponse{ 
    std::string statuscode;
    std::string statusmsg;
    std::map<std::string, std::string> headers;
    std::string body;
    public:
    std::string frameHttpResponse(std::string statuscode,
                                  std::string statusmsg, 
                                  std::map<std::string, std::string> headers,
                                  std::string body,
                                  std::string mimetype);
};

std::string
HttpResponse::frameHttpResponse(std::string statuscode,
                                std::string statusmsg, 
                                std::map<std::string, std::string> headers, 
                                std::string body,
                                std::string mimetype)
{
    headers["content-type"] = mimetype;
    headers["content-length"] = std::to_string(body.length());
    std::ostringstream buffer;
    buffer << "HTTP/1.1 " << statuscode << " " << statusmsg << "\r\n";
    
    for(auto x : headers) {
        buffer << x.first << ": " << x.second << "\r\n";
    }
    
    buffer << "\r\n" << body;
    return buffer.str();
}

class Server
{
public:
    Server(std::string port) : _port(port) { }
    ~Server() { stop(); }

    int start()
    {
        int server_socket_fd; //file descriptior of server socket.
        struct sockaddr_in server_addr; //structure for server address it helps to config and bind server socket.
        server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        
        if (server_socket_fd < 0) {
            std::cerr << "Failed to create server socket." << std::endl;
            return -1;
        }

        //config server socket.
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(std::stoi(_port));
        
        //INADDR_ANY allows the server to listen on all available network interfaces.
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        //bind  socket to port.
        if (bind(server_socket_fd,(struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind server socket." << std::endl;
            return -1;
        }

        //listens on socket.
        if (listen(server_socket_fd, 5) < 0) {
            std::cerr << "Failed to listen on server socket." << std::endl;
            return -1;
        }
        
        std::cout << "Server started on port : " << _port << std::endl;

        return server_socket_fd;
    }
    
    void stop()
    {
        //TODO
        //close the server socket.
    }
    
private:
    std::string _port;
};

void handleClient(int client_socket_fd)
{
    char client_req_buffer[1024];
    
    read(client_socket_fd, client_req_buffer, 1024);
    HttpRequest req = HttpRequest();
    
    req.parseRequest(client_req_buffer);
    std::string mimetype = req.getMimeType(req.path);
    
    HttpResponse res= HttpResponse();
    std::string body = req.readHtmlFile(req.path);
    std::string response = res.frameHttpResponse("200","OK",req.headers,body,mimetype);

    write(client_socket_fd, response.c_str(), response.length());
    
    close(client_socket_fd);
}

int main(int argc, char* argv[])
{
    std::string port = (argc > 1) ? argv[1] : "3000";
    
    Server server = Server(port);
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int s_fd;
    s_fd = server.start();
    client_addr_size = sizeof(struct sockaddr_in); 

    while (1) {
        int client_socket_fd = accept(s_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if(client_socket_fd < 0){
            std::cerr << "Failed to accept client request." << std::endl;
            exit(1);
        }
       
        // Create a new thread to handle the client
        std::thread clientThread(handleClient, client_socket_fd);
        clientThread.detach(); // Detach the thread to allow it to run independently
    }

    return 0;
}
