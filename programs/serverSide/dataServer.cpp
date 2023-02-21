#include "server.h"

int main(int argc, char *argv[]) {
    if (argc != NUM_OF_ARGS) {
        printf("Please give -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n");
        exit(EXIT_FAILURE);
    }

    int port;
    int pool_size;
    int queue_size;
    int block_size;
    for(int i=1; i<8; i = i+2){
        if(strcmp(argv[i], "-p") == 0)
            port = atoi(argv[i+1]);
        else if(strcmp(argv[i], "-s") == 0)
            pool_size = atoi(argv[i+1]);
        else if(strcmp(argv[i], "-q") == 0)
            queue_size = atoi(argv[i+1]);
        else
            block_size = atoi(argv[i+1]);
    }


    Server* server = new Server(port, pool_size, queue_size, block_size);
    server->set_up_server();
    server->run_server();
}