/* inet_str_client.c: Internet stream sockets client */
#include <stdio.h>
#include <iostream>
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <unistd.h> /* read, write, close */
#include <netdb.h> /* gethostbyaddr */
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 512
#define DIR_PATH "../../serverOutput/"
#define DIR_PEMRS 0777
#define FILE_PEMRS 0777

#pragma once

class Client{

    private:
        int port, sock, i;
        const char* server_ip;
        const char* dir_name;
        struct sockaddr_in server;
        struct sockaddr *serverptr = (struct sockaddr*)&server;
        struct hostent *rem;
        struct in_addr addr; // to get the server address

    public:
        Client(int _port, const char* _server_ip, const char* _dir_name);
        ~Client();

        void set_up_client();
        void run_client();
        
    private:
        int get_line(char* buf, ssize_t bytes, int start, std::string& line);
        std::string create_dir(std::string path);
        void append_file(std::string path, std::string content);
};