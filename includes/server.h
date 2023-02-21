#include <stdio.h>
#include <iostream>
#include <sys/wait.h> /* sockets */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <netdb.h> /* gethostbyaddr */
#include <unistd.h> /* fork */
#include <stdlib.h> /* exit */
#include <ctype.h> /* toupper */
#include <dirent.h>
#include <string.h>
#include <queue>
#include <map>
#include <sys/stat.h>
#include <pthread.h>

#define NUM_OF_ARGS 9
#define BUF_SIZE 512

typedef std::pair <std::string, int> Node;

#pragma once

class Server;
//this struct helps as pass the arguments that we want on the communicator
typedef struct{
    Server* server;
    int* sock;
}com_args;

class Server{

    private:
        int port, pool_size, queue_size, block_size, sock;

        struct sockaddr_in server, client;
        socklen_t clientlen;
        struct sockaddr *serverptr=(struct sockaddr *)&server;
        struct sockaddr *clientptr=(struct sockaddr *)&client;
        struct hostent *rem;

    public:
        std::queue<Node> queue;
        std::map<int,pthread_mutex_t*> map;
        pthread_mutex_t queue_mutex;
        pthread_mutex_t map_mutex;
        pthread_cond_t cond_queue_nonempty;   //for queue
        pthread_cond_t cond_queue_nonfull;    //for queue
        pthread_cond_t cond_map_nonempty;   //for map

    public:
        Server(int _port, int _pool_size, int _queue_size, int _block_size);
        ~Server();
        void set_up_server();
        void run_server();
        void send_message(Node node);
        void read_directory(const char* dir_name, int socket);
        int get_dir_size(const char* dir_name);
        int get_file_size(const char* dir_name);
        char read_file(FILE* fptr, std::string& block);
};