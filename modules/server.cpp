#include "server.h"

static void perror_exit(const char *message){
    perror(message);
    exit(EXIT_FAILURE);
}

// Test function to print the queue
/* Not needed in the final project
static void printQueue(std::queue<Node> queue){
    std::queue<Node> temp = queue;

    std::cout << "Current files in queue" << std::endl;
	while (!temp.empty()){
        Node node = temp.front();
        std::cout << "<" << node.first << ", " << node.second << ">" << std::endl;
		temp.pop();
	}
	std::cout << std::endl;
}
*/

Server::Server(int _port, int _pool_size, int _queue_size, int _block_size){
    std::cout << "Server created" << std::endl;
    port = _port;
    pool_size = _pool_size;
    queue_size = _queue_size;
    block_size = _block_size;
}

Server::~Server(){
    std::cout << "Server deleted" << std::endl;
}

void* worker_job(void* ptr){
    Server* server = (Server*)ptr;
    //workers will stay alive all the time
    while(true){

        //try to lock queue to get a file to copy
        pthread_mutex_lock(&server->queue_mutex);
        while(server->queue.empty())
            pthread_cond_wait(&server->cond_queue_nonempty, &server->queue_mutex);

        //get the node with the filename
        Node node = server->queue.front();
        server->queue.pop();

        //lock the map
        pthread_mutex_lock(&server->map_mutex);

        //block here until we can lock the client mutex again
        while(server->map.find(node.second) == server->map.end())
            pthread_cond_wait(&server->cond_map_nonempty, &server->map_mutex);
        
        std::map<int, pthread_mutex_t*>::iterator it = server->map.find(node.second);
        int tsock = it->first;
        pthread_mutex_t* tmutex = it->second;

        //server will work with a single client
        pthread_mutex_lock(tmutex);
        //we remove the pair
        server->map.erase(tsock);

        //we let other workers try to lock the map and get a client mutex
        pthread_mutex_unlock(&server->map_mutex);
        
        //unlock to mutex and signal that there is space inside the queue
        pthread_mutex_unlock(&server->queue_mutex);
        pthread_cond_signal(&server->cond_queue_nonfull);
        
        //send the file to the client
        server->send_message(node);

        char buf[BUF_SIZE];
        memset(buf,0,BUF_SIZE);

        //block on read until the client reads all the file
        //waits for clients response
        if(read(node.second, buf, BUF_SIZE) < 0)
            perror_exit("read");
        
        /* 
            Buf will have the values:
                * "OK" -> file is copied
                * "END" -> all the files are copied
        */

        //if we finished with all the clients request we close the socket
        if(strcmp(buf, "END") == 0)
            close(node.second);

        //now we let the other workers contest for the client mutex
        pthread_mutex_unlock(tmutex);

        //if we finished with the client we destroy his mutex
        if(strcmp(buf, "END") == 0){
            pthread_mutex_destroy(tmutex);
        }
        //else we insert it back to the map for the next worker
        else{
            //first lock the map
            pthread_mutex_lock(&server->map_mutex);
            server->map.insert({tsock,tmutex});

            //inform that a mutex is inserted in the map
            pthread_cond_broadcast(&server->cond_map_nonempty);
            pthread_mutex_unlock(&server->map_mutex);
        }
    }
}

void* communicator_job(void* args){

        char buf[BUF_SIZE];
        memset(buf,0,BUF_SIZE);

        //get the args
        com_args* communicator_args = (com_args*)args;
        int newsock = *communicator_args->sock;
        Server* server = communicator_args->server;

        //for each new client we create his mutex
        pthread_mutex_t client_mutex;
        pthread_mutex_init(&client_mutex, NULL);
        server->map.insert({newsock, &client_mutex});

        //read the name of the dir to copy
        if(read(newsock, buf, BUF_SIZE) < 0)
            perror_exit("read");

        //first we want the size of the dir
        int dir_size = server->get_dir_size(buf);

        //we send the size of the wanted directory to the client
        std::string dir_size_s = std::to_string(dir_size).append("\n");
        if(write(newsock, dir_size_s.c_str(), strlen(dir_size_s.c_str())) < 0)
            perror_exit("write");

        //now we read the files names and add them to the queue
        server->read_directory(buf, newsock);

        //clear the buf
        memset(buf,0,BUF_SIZE);
        free(communicator_args->sock);
        free(communicator_args);

        //we exit the thread
        pthread_exit(0);
}

void Server::set_up_server(){ 
    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");

    server.sin_family = AF_INET; /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port); /* The given port */

    //to handle the bind already in use error
    int flag = 1;
    if(-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
        perror_exit("setsockopt fail");  

    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");

    /* Listen for connections */
    if (listen(sock, SOMAXCONN) < 0) 
        perror_exit("listen");
    printf("Listening for connections to port %d\n", port);
    
    //initialize mutex and cond vars
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&map_mutex, NULL);
    pthread_cond_init(&cond_queue_nonempty, NULL);
    pthread_cond_init(&cond_queue_nonfull, NULL);
    pthread_cond_init(&cond_map_nonempty, NULL);
    //create the workers
    printf("Creating workers\n");
    for(int i=0; i<pool_size; i++){
        pthread_t worker;
        pthread_create(&worker, 0, worker_job , this);
        pthread_detach(worker);
    }
}

void Server::run_server(){
    clientlen = sizeof(client);

    //server will run all the time / waiting for new clients
    while(true){
        /* accept connection */
        int* newsock = (int*)malloc(sizeof(int));
        if ((*newsock = accept(sock, clientptr, &clientlen)) < 0) 
            perror_exit("accept");
        
        /* Find client's name */
        if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr,sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL){
            herror("gethostbyaddr");
            exit(1);
        }

        pthread_t communicator;
        //create communicator arguments
        com_args *args = (com_args*)malloc(sizeof (*args));
        args->server = this;
        args->sock = newsock;

        pthread_create(&communicator, 0, communicator_job, args);
        pthread_detach(communicator);
    }

}

int Server::get_dir_size(const char* dir_name){
    DIR *folder;
    struct dirent *entry;
    int files = 0;

    folder = opendir(dir_name);
    if(folder == NULL)
        perror_exit("Unable to read directory");
    
    while((entry=readdir(folder))){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        //if we find a sub directory call recursively the function
        if(entry->d_type == 4){
            std::string sub_dir;
            sub_dir.append(dir_name).append(entry->d_name).append("/");
            files += this->get_dir_size(sub_dir.c_str());
        }
        else{
            files++;
        }
    }
    closedir(folder);

    return files;
}

//reads the specified dir and adds the files to the queue
void Server::read_directory(const char* dir_name, int newsock){
    DIR *folder;
    struct dirent *entry;

    folder = opendir(dir_name);
    if(folder == NULL)
        perror_exit("Unable to read directory");
    
    while((entry=readdir(folder))){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        //if we find a sub directory call recursively the function
        if(entry->d_type == 4){
            std::string sub_dir;
            sub_dir.append(dir_name).append(entry->d_name).append("/");
            this->read_directory(sub_dir.c_str(), newsock);
        }
        else{
            Node node;
            //we put the path to the file
            node.first.append(dir_name).append(entry->d_name);

            //we append to add the file size
            int file_size = this->get_file_size(node.first.c_str());
            node.first.append("\n").append(std::to_string(file_size)).append("\n");

            node.second = newsock;

            //try to lock queue mutex to add new file in the queue
            pthread_mutex_lock(&queue_mutex);
            while(queue.size() == (uint)queue_size)
                pthread_cond_wait(&cond_queue_nonfull, &queue_mutex);
            
            queue.push(node);
            
            //unlock queue mutex and signal that the queue is non empty
            pthread_mutex_unlock(&queue_mutex);
            pthread_cond_signal(&cond_queue_nonempty);

            printf("\tFile : %s to socket %d\n", entry->d_name, newsock);
        }
    }
    closedir(folder);
}

char Server::read_file(FILE* ptr, std::string& block){
    char ch;
    int counter = 0;

    while((ch = fgetc(ptr)) != EOF){
        block += ch;
        counter ++;

        if(counter == block_size)
            break;
    }
    
    return ch;
}

int Server::get_file_size(const char* path){
    FILE *fptr;
    long file_size;

    if((fptr = fopen(path,"r")) == NULL)
        perror_exit("file open");

    fseek(fptr, 0L, SEEK_END);
    file_size = ftell(fptr);
    rewind(fptr);

    if(fclose(fptr) != 0)
        perror_exit("file close");
    
    return file_size;
}

void Server::send_message(Node node){

    std::string path = node.first;
    int tsocket = node.second;

    //first we send the filename and its size
    if(write(tsocket, path.c_str(), strlen(path.c_str())) < 0)
        perror_exit("write");
    
    //hold the filename
    size_t pos = path.find("\n");
    std::string filename = path.substr(0,pos);

    //then we start reading the file and sending its content
    FILE* fptr;
    std::string block;

    //open file
    if((fptr = fopen(filename.c_str(),"r")) == NULL)
        perror_exit("file open");
    
    //read blocks until EOF
    char c;
    do{
        c = read_file(fptr,block);
        if (write(tsocket, block.c_str(), strlen(block.c_str())) < 0)
            perror_exit("write");
        block.clear();
    }while(c != EOF);
    
    //close file
    if(fclose(fptr) != 0)
        perror_exit("file close");

}