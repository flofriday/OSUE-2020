/**
 * @file server.c
 * @author Jonny X <e12345678@student.tuwien.ac.at>
 * @date 05.01.2021
 * version: 1.0.0
 *
 * @brief This program is an implementation of a http server (version 1.1)
 **/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h> 
#include <assert.h>

static char *prog_name;
volatile sig_atomic_t quit = 0;


/**
 * @brief
 * Handles the signal if it occurres.
 * 
 * @details
 * Gets called when an ia signal occures. Sets quit to 1 so the main while loop is exited.
 * @param signal the signal which was received
**/
void handle_signal(int signal) { quit = 1; }


/**
 * @brief
 * Prints the usage format to explain which arguments are expected. 
 * After that the program ist terminated with an error code.
 * 
 * @details
 * Gets called when an input error by the user is detected.
**/
void usage(void)
{
    fprintf(stderr, "[%s] Usage: %s [-p PORT] [-i INDEX] DOC_ROOT\n", prog_name, prog_name);
    exit(EXIT_FAILURE);
}

/**
 * @brief
 * Parses the header of the client
 * 
 * @details
 * Parses the header and returns the status code.
 * @param connect_file 
 * @param buffer 
 * @param buffer_cap 
 * @param index_filename 
 * @param doc_dir 
 * @param full_path 
 * @return Returns the status code
**/
int parse_header(FILE *connect_file, char *buffer, size_t *buffer_cap, char *index_filename, char *doc_dir, char* full_path){
    char first_line[strlen(buffer) + 1];
    strcpy(first_line, buffer);
    
    // split by space. eg: GET /index.html HTTP/1.1 -> 
    // request_method="GET" 
    // path="/index.html" 
    // version="HTTP/1.1" 
    // empty="\r\n"
    char* request_method = strtok(first_line, " ");
    char* path = strtok(NULL, " ");
    char* version = strtok(NULL, " ");
    char* empty = strtok(NULL, "\r\n");    
    //fprintf(stderr, "request_method=%s path=%s version=%s empty=%s\n", request_method, path, version, empty);

    strcpy(full_path, doc_dir);
    strcat(full_path, path);
    //fprintf(stderr, "FULL PATH=%s\n", full_path);

    //if last symbol is / -> add index.html as the file (directory was specified)
    if(path[strlen(path)-1] == '/'){
        strcat(full_path, "index.html");
    }

    
    //fprintf(stderr, "version=(%s)\n", version);
    //return the appropriate status code
    if(request_method == NULL || path == NULL || version == NULL || empty != NULL){
        return 400;
    }
    else if(strcmp(version, "HTTP/1.1\r\n") != 0){
        return 400;
    }
    else if(strcmp(request_method, "GET") != 0){
        return 501;
    }
    return 200;
}

/**
 * @brief
 * Returns the file size of the specified path
 * 
 * @details
 * Returns the file size of the file at the specified path. If an error occures -1 is returned.
 * @param path The path to the file
 * @return Returns the file size. If the file size can't be fetched -1 is returned.
**/
int get_file_size(char *path){
    struct stat buffer;
    int status;

    status = stat(path, &buffer);
    if(status == 0) {
        return buffer.st_size;
    }

    return -1;
}


/**
 * @brief
 * Returns the current time
 * 
 * @details
 * Returns the current time formated according to GMT format. Example: Sun, 11 Nov 18 22:55:00 GMT
 * @return Returns the current time as a char*. This pointer needs to be freed by the caller.
**/
char* get_current_time(){
    char *buffer = malloc(100);
    time_t rawtime = time(NULL);
    struct tm *time_info = gmtime(&rawtime);

    strftime(buffer, 100, "%a, %d %b %y %T %Z", time_info);
    return buffer;
}

/**
 * @brief
 * Writes to the header
 * 
 * @details
 * Writes the header to the specified file with the status code and specified path
 * @param status_code the status code which should be written to the header
 * @param path the path to the file which is used to get the file size and extension of the file.
 * @param file FILE* to which the header is written
**/
void write_header(int status_code, char* path, FILE* file){
    int content_length = get_file_size(path);
    char *date = get_current_time();
    char *status_message = "OK";
    char *content_type = "";

    assert(status_code == 200);
    //fprintf(stderr, "[%s] path=%s\n", prog_name, path);

    char *extension = strrchr(path, '.') + 1;

    // set content type according to the extension type of the path
    if(strcmp(extension, "html") == 0 || strcmp(extension, "htm") == 0){
        content_type = "Content-Type: text/html\r\n";
    }
    else if(strcmp(extension, "css") == 0){
        content_type = "Content-Type: text/css\r\n";
    }
    else if(strcmp(extension, "js") == 0){
        content_type = "Content-Type: application/javascript\r\n";
    }
    
    if(fprintf(file, "HTTP/1.1 %d %s\r\nDate: %s\r\n%sContent-Length: %d\r\nConnection: close\r\n\r\n", status_code, status_message, date, content_type, content_length) < 0){
        fprintf(stderr, "[%s] Error fprintf failed\n", prog_name);
    }
    free(date);
    fflush(file);
}

/**
 * @brief
 * Writes to the header in case of an status code other than 200
 * 
 * @details
 * Writes to the header in case of an status code other than 200. The specified file and status code 
 * are used for that
 * @param status_code the status code which should be written to the header
 * @param file FILE* to which the header is written
**/
void write_error_header(int status_code, FILE* file){
    char* status_message = "";
    switch(status_code){
        case 400:
            status_message = "Bad Request";
            break;
        case 404:
            status_message = "Not Found";
            break;
        case 501:
            status_message = "Not Implemented";
            break;
        default:
            assert(false);
            break;
    }


    if(fprintf(file, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n", status_code, status_message) < 0){
        fprintf(stderr, "[%s] Error fprintf failed\n", prog_name);
    }
    fflush(file);
}

/**
 * @brief
 * Writes the content to the connection
 * 
 * @details
 * Reads and writes the content as binary data.
 * @param input_file The FILE* from which should be read.
 * @param socket_file The FILE* to which should be written. This is the connection file which msut be opened before
**/
void write_content(FILE *input_file, FILE *socket_file){
    // 8k is a somewhat reasonable buffer size
    size_t buffer_size = 8 * 1024;
    char buffer[buffer_size];
    size_t read = 0;
    while((read = fread(buffer, sizeof(char), buffer_size, input_file)) != 0){
        fwrite(buffer, sizeof(char), read, socket_file);
    }
    fflush(socket_file);
}


/**
 * @brief
 * Accepts the next pending connection
 * 
 * @details
 * Accepts the next connection and sets the FILE** connect_file to the new connection
 * @param socket_fd The socket filedescriptor which is used to accept the next connection
 * @param connect_file The FILE* to the new connection
 * @return Returns -1 on failure. Returns 0 on success or if it is interrupted by a signal.
**/
int open_next_connection(int socket_fd, FILE **connect_file){
    int connect_fd = accept(socket_fd, NULL, NULL);
    if (connect_fd == -1){
        if(errno == EINTR){
            return 0;
        }
        else{
            fprintf(stderr, "[%s] Error accept failed\n", prog_name);
            return -1;
        }

    }

    *connect_file = fdopen(connect_fd, "r+");
    if(*connect_file == NULL){
        close(connect_fd);
        fprintf(stderr, "[%s] Error fdopen failed\n", prog_name);
        return -1;
    }

    return 0;
}


/**
 * @brief
 * Sets up the server with the provided port.
 * 
 * @details
 * OPens a new connection to which clients can connect and request files
 * @param port The port where the server should be set up.
 * @return Returns the socket_fd on success and -1 on failure
**/
int setup_server(char *port){
    //setup addrinfo struct with host and port information
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &ai);
    if (res != 0) {
        fprintf(stderr, "[%s] Error getaddrinfo failed\n", prog_name);
        return -1;
    }

    int socket_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (socket_fd < 0) {
        fprintf(stderr, "[%s] Error socket failed\n", prog_name);
        return -1;
    }

    //option to reuse port immediatley
    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(socket_fd, ai->ai_addr, ai->ai_addrlen) < 0) {
        fprintf(stderr, "[%s] Error bind failed\n", prog_name);
        return -1;
    }

    if (listen(socket_fd, 10) < 0){
        fprintf(stderr, "[%s] Error listen failed\n", prog_name);
        return -1;
    }

    
    freeaddrinfo(ai);

    return socket_fd;
}



/**
 * Program entry point
 * @brief
 * This program servers as a http server (version 1.1)
 * 
 * @details
 * This program servers as a http server (version 1.1). It parses the arguments and sets up the connection accordingly.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE.
**/
int main(int argc, char *argv[])
{
    char *port;
    char *index_filename;
    char *doc_dir;
    prog_name = argv[0];
    int opt;
    bool p_flag = false;

    //assign default values
    port = "8080";
    index_filename = "index.html";

    while ((opt = getopt(argc, argv, "p:i:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (p_flag == true)
            {
                usage();
            }
            p_flag = true;

            //check if strtol is successfull
            char *endpointer = NULL;
            int parsed_port = strtol(optarg, &endpointer, 10);
            if((parsed_port == 0 && (errno != 0 || endpointer == optarg)) || *endpointer != '\0'){
                usage();  
            }
            port = optarg;
            break;
        case 'i':
            if (strcmp(index_filename, "index.html") != 0)
            {
                usage();
            }

            index_filename = optarg;
            break;
        default:
            usage();
            break;
        }
    }

    // If no or more than 1 positional argument (=DOC_ROOT) was/were specified: 
    // Call usage and exit. Otherwise copy the argument
    if (optind != argc - 1)
    {
        usage();
    }
    doc_dir = argv[optind];
    fprintf(stderr, "[%s] Parsed Arguments: port=%s index_filename=%s docdir=%s\n", prog_name, port, index_filename, doc_dir);

    //setup up signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int socket_fd = setup_server(port);
    if(socket_fd == -1){
        fprintf(stderr, "[%s] Could not open open socket\n", prog_name);
        exit(EXIT_FAILURE);
    }
    char *buffer = NULL;
    size_t buffer_cap = 0;

    while(!quit){
        FILE* connect_file = NULL;
        int connection = open_next_connection(socket_fd, &connect_file);
        if(connection == -1){
            fprintf(stderr, "[%s] Error opening connection\n", prog_name);
            continue;
        }
        // Signal occured. continue -> quit is 1 and exit the loop
        if(connect_file == NULL){
            continue;
        }

        // Read first line
        if(getline(&buffer, &buffer_cap, connect_file) == -1){
            fprintf(stderr, "[%s] Error getline failed (%s)\n", prog_name, strerror(errno));
            fclose(connect_file);
            continue;
        }
        
        char full_path[strlen(doc_dir) + strlen(buffer) + strlen(index_filename)];
        strcpy(full_path, "");
        int status_code = parse_header(connect_file, buffer, &buffer_cap, index_filename, doc_dir, full_path);

        // Read the rest after the header but as we dont need the information we just discard it.
        while(getline(&buffer, &buffer_cap, connect_file) != -1){
            if(strcmp(buffer, "\r\n") == 0){
                break;
            }
        }
        //fprintf(stderr, "FULL PATH=%s\n", full_path);

        // Open the specified file which should be transmitted
        FILE *input_file = fopen(full_path, "r");
        if(status_code == 200){
            if(input_file == NULL){
                status_code = 404;
                //fprintf(stderr, "[%s] Status code 404: File (%s) not found (%s)\n", prog_name, full_path, strerror(errno));
            }
        }
        fprintf(stderr, "[%s] Status-code [%d], File-path (%s) \n", prog_name, status_code, full_path);

        // Write the normal header and content if the status code is 200. Otherwise write the error header and no content
        if(status_code == 200){
            write_header(status_code, full_path, connect_file);
            write_content(input_file, connect_file);
            //write_content(input_file, stderr);
        }
        else{
            write_error_header(status_code, connect_file);
        }


        // Free resources
        if(input_file != NULL){
            fclose(input_file);
        }
        fclose(connect_file);
    }

    // Free resources
    free(buffer);
    
    exit(EXIT_SUCCESS);
}