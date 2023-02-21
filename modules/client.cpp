#include "client.h"

static void perror_exit(const char *message){
    perror(message);
    exit(EXIT_FAILURE);
}

Client::Client(int _port, const char* _server_ip, const char* _dir_name){
    std::cout << "Client created" << std::endl;
    port = _port;
    server_ip = _server_ip;
    dir_name = _dir_name;
}

Client::~Client(){
    std::cout << "Client deleted" << std::endl;
}

void Client::set_up_client(){

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");

    /* Find server address */
    if(inet_aton(server_ip, &addr) == 0){
        perror_exit("inet_aton");
    }

    if((rem = gethostbyaddr(&addr, sizeof(addr), AF_INET) ) == NULL){
        herror("gethostbyaddr");
        exit(1);
    }

    server.sin_family = AF_INET; /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port); /* Server port */

    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
        perror_exit("connect");

    printf("Connecting to %s port %d\n", server_ip, port);
    
}

std::string Client::create_dir(std::string path){      
    //extract dir_name
    std::string s = "../";
    size_t pos = 0;
    while((pos = path.find(s)) != std::string::npos)
        path.erase(pos, s.length());
    
    //create dir
    s = "/";
    pos = 0;
    size_t file_pos = 0;
    std::string dir_path;
    while ((pos = path.find(s, pos)) != std::string::npos) {
        dir_path = DIR_PATH + path.substr(0,pos) + "/";

        if(mkdir(dir_path.c_str(), DIR_PEMRS) != 0)
            if(errno != EEXIST)
                perror_exit("mkdir");

        pos += s.length();
        file_pos = pos;
    }

    //create file
    std::string file_path = dir_path + path.substr(file_pos);

    //here we create or we truncate the file if already exists
    FILE * fptr;
    fptr = fopen(file_path.c_str(), "w+");

    if(fptr == NULL)
        perror_exit("fopen");

    //and we close it again
    if(fclose(fptr) == EOF)
        perror_exit("fclose");

    return file_path;
}

void Client::append_file(std::string path, std::string content){
    FILE * fptr;
    fptr = fopen(path.c_str(), "a");

    if(fptr == NULL)
        perror_exit("fopen");

    if(fputs(content.c_str(), fptr) == EOF)
        perror_exit("fputs");

    if(fclose(fptr) == EOF)
        perror_exit("fclose");
}

void Client::run_client(){

    printf("Directory path: %s\n", dir_name);

    if (write(sock, dir_name, strlen(dir_name)) < 0)
        perror_exit("write");
    
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    int dir_size = -1;
    int file_size = -1;
    int bytes_read = 0;
    int bytes_to_read = 0;
    int files_read = 0;

    std::string filename;
    std::string file_path;
    ssize_t read_size = 0;
    while((read_size = read(sock, buf, BUF_SIZE)) > 0){

        int start = 0;
        std::string line;
        
        //first line that we read
        start = this->get_line(buf, read_size, start, line);

        //fist line from message will be the dir size
        if(dir_size == -1){
            dir_size = std::stoi(line);
            std::cout << "Dir size:" << dir_size << std::endl;
        }
        //second line from message will be the filename
        else if(filename.empty()){
            filename = line;
            std::cout << "file name:" << filename;
            bytes_read = 0;

            //create the dir and add the file inside
            file_path = this->create_dir(filename);
        }
        //third line will be the size of the file
        else if(file_size == -1){
            file_size = std::stoi(line);
            std::cout << "file size:" << file_size << "\n"  << std::endl;
            bytes_to_read = file_size;

            //if file is empty
            if(bytes_to_read == 0){
                filename.clear();
                file_size = -1;
                files_read ++;

                //if its the last file send END message
                if(files_read == dir_size){
                    if(write(sock, "END", strlen("END")) < 0)
                        perror_exit("write");
                }
                //else just OK to verify that client read the file
                else{
                    if(write(sock, "OK", strlen("OK")) < 0)
                        perror_exit("write");
                }
            }
        }
        //or we read the text from file
        else{
            //add content to the file
            this->append_file(file_path, line);
            bytes_read += strlen(line.c_str());

            //we finished reading the file
            if(bytes_read == bytes_to_read){
                filename.clear();
                file_size = -1;
                files_read ++;
                
                //if its the last file send END message
                if(files_read == dir_size){
                    if(write(sock, "END", strlen("END")) < 0)
                        perror_exit("write");
                }
                //else just OK to verify that client read the file
                else{
                    if(write(sock, "OK", strlen("OK")) < 0)
                        perror_exit("write");
                }
            }
        }

        //we continue with the other lines
        if(start != read_size)
            do{
                start = this->get_line(buf, read_size, start, line);

                if(filename.empty()){
                    filename = line;
                    std::cout << "file name:" << filename;
                    bytes_read = 0;
                    file_path = this->create_dir(filename);
                }
                else if(file_size == -1){
                    file_size = std::stoi(line);
                    std::cout << "file size:" << file_size << "\n" << std::endl;
                    bytes_to_read = file_size;

                    //if file is empty
                    if(bytes_to_read == 0){
                        filename.clear();
                        file_size = -1;
                        files_read ++;

                        //if its the last file send END message
                        if(files_read == dir_size){
                            if(write(sock, "END", strlen("END")) < 0)
                                perror_exit("write");
                        }
                        //else just OK to verify that client read the file
                        else{
                            if(write(sock, "OK", strlen("OK")) < 0)
                                perror_exit("write");
                        }
                    }
                }
                else{
                    this->append_file(file_path, line);
                    bytes_read += strlen(line.c_str());

                    //we finished reading the file
                    if(bytes_read == bytes_to_read){
                        filename.clear();
                        file_size = -1;
                        files_read ++;

                        //if its the last file send END message
                        if(files_read == dir_size){
                            if(write(sock, "END", strlen("END")) < 0)
                                perror_exit("write");
                        }
                        //else just OK to verify that client read the file
                        else{
                            if(write(sock, "OK", strlen("OK")) < 0)
                                perror_exit("write");
                        }
                    }
                }

            }while(start != read_size);

        memset(buf, 0, BUF_SIZE);

        //if we finished all the files leave the loop
        if(files_read == dir_size)
            break;
    }

    if(read_size == -1)
        perror_exit("read");

    close(sock); /* Close socket and exit */
}

int Client::get_line(char* buf, ssize_t read_size, int start, std::string& line){
    line.clear();
    if(read_size > 0){
        for (int i=start; i < read_size; i++){
            line += buf[i];
            if (buf[i] == '\n'){
                return ++i;
            }
        }
    }
    return read_size;
}