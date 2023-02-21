#include "client.h"

int main(int argc, char *argv[]) {

    if (argc != 7) {
        printf("Please give -i <server_ip> -p <server_port> -d <directory>\n");
        exit(EXIT_FAILURE);
    }

    const char* server_ip;
    int port;
    const char* dir_name;
    for(int i=1; i<6; i = i+2){
        if(strcmp(argv[i], "-i") == 0)
            server_ip = argv[i+1];
        else if(strcmp(argv[i], "-p") == 0)
            port = atoi(argv[i+1]);
        else
            dir_name = argv[i+1];
    }

    Client* client = new Client(port, server_ip, dir_name);
    client->set_up_client();
    client->run_client();
    delete(client);
}
