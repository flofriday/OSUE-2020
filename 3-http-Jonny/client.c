/**
 * @file client.c
 * @author Jonny X <e12345678@student.tuwien.ac.at>
 * @date 05.01.2021
 * version: 1.0.0
 *
 * @brief This program servers as a http client (version 1.1)
 **/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>

static char *prog_name;

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
    fprintf(stderr, "[%s] Usage: %s [-p PORT] [ -o FILE | -d DIR ] URL\n", prog_name, prog_name);
    exit(EXIT_FAILURE);
}

/**
 * @brief
 * parses the provided arguments
 * 
 * @details
 * parses the provided arguments and sets the provided arrays with the correct formatted data
 * @param path_opt the path to the file or directory which the user specified as an argument
 * @param url_opt the requested url which the user specified as an argument
 * @param host a pointer to an array which was created in main and is filled with the host as strign by this function
 * @param output_path a pointer to an array which was created in main and is filled with the defined 
 *                     output path for the file as string by this function
 * @param request_path a pointer to an array which was created in main and is filled with the defined 
 *                     requested path after the host by this function
 * @param d_flag signals whether a directory (=true) or a file (=false) was set as the path option
**/
void parse_arguments(char *path_opt, char *url_opt, char* host, char* output_path, char* request_path, bool d_flag){
    strcpy(output_path, path_opt);

    // Add / if output path does not end with /
    if (d_flag == true && output_path[strlen(output_path) - 1] != '/')
    {
        strcat(output_path, "/");
    }
    // Add index.html if directory path does end with /
    if (d_flag == true && url_opt[strlen(url_opt) - 1] == '/')
    {
        strcat(output_path, "index.html");
    }
    else if (d_flag == true)
    {
        // +1 to not get 2 //
        strcat(output_path, strrchr(url_opt, '/') + 1);
    }

    // Check if url starts with http://
    if (strncmp(url_opt, "http://", strlen("http://")) != 0)
    {
        usage();
    }
    // Remove http:// to get the host
    strcpy(host, url_opt + strlen("http://"));

    // Get file path
    char *request_file = strpbrk(host, ";/?:@=&");
    if(request_file == NULL){
        strcpy(request_path, "/");
    }
    else{
        strcpy(request_path, request_file);
        strcpy(request_file, "");
    }
    

}

/**
 * @brief
 * reads, parses and validates the header
 * 
 * @details
 * reads the header line by line and check if the version matches. Parses the status code and message and cheks them.
 * @param socket_file The FIle* from which should be read.
 * @return Returns 0 if everything was successfull. Otherwise returns the exit code with which the program should exit.
**/
int read_header_and_validate(FILE *socket_file)
{
    char *buffer = NULL;
    size_t buffer_cap = 0;

    if (getline(&buffer, &buffer_cap, socket_file) == -1)
    {
        fprintf(stderr, "[%s] Error getline failed (%s)\n", prog_name, strerror(errno));
        free(buffer);
        return 1;
    }

    char* http_version = strtok(buffer, " ");
    char* status_code = strtok(NULL, " ");
    char* status_message = strtok(NULL, "\r");
    //fprintf(stderr, "http-version=%s status=%s text-status=%s\n", http_version, status_code, status_message);

    if (strncmp(http_version, "HTTP/1.1", strlen("HTTP/1.1")) != 0)
    {
        fprintf(stderr, "[%s] Protocol error!\n", prog_name);
        free(buffer);
        return 2;
    }
    else if (strncmp(status_code, "200", strlen("200")) != 0)
    {
        fprintf(stderr, "[%s] %s %s\n", prog_name, status_code, status_message);
        free(buffer);
        return 3;
    }

    while (getline(&buffer, &buffer_cap, socket_file) != -1)
    {
        if (strncmp(buffer, "\r\n", strlen("\r\n")) == 0)
        {
            break;
        }
    }

    free(buffer);
    return 0;
}

/**
 * @brief
 * reads the content part of the http response
 * 
 * @details
 * Reads the content as binary data.
 * @param socket_file The FILE* from which should be read.
 * @param output_file The FILE* to which sholuld be written. Could be a file or stdout.
**/
void read_content(FILE *socket_file, FILE *output_file){
    // 8k is a somewhat reasonable buffer size
    size_t buffer_size = 8 * 1024;
    char buffer[buffer_size];
    size_t read = 0;
    while((read = fread(buffer, sizeof(char), buffer_size, socket_file)) != 0){
        fwrite(buffer, sizeof(char), read, output_file);
    }
    fflush(output_file);
}

/**
 * @brief
 * sends the initial GET request
 * 
 * @details
 * Request the specified file by sending a GET request to the server
 * @param host The host to which the request is sent.
 * @param request_path The relative path of the requested file/data.
 * @param socket_file The FILE* with the open connection to which should be written.
**/
void send_get_request(char* host, char* request_path, FILE *socket_file){
    fprintf(socket_file, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", request_path, host);
    fflush(socket_file);
}

/**
 * @brief
 * sets up the connection with the server
 * 
 * @details
 * Sets up the connection with the server and returns the appropriate file descriptor which is then used
 * to open the FILE* and operate with the standard I/O functions
 * @param port The port of the server to which the client should connect
 * @param host The host to which the client should connect.
 * @param request_path The relative file path after the host.
 * @return Returns the file descriptor if the connection was successfull. Otherwise -1 is returned.
**/
int set_up_connection(char* port, char* host, char* request_path){
    //setup addrinfo struct with host and port information
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //setup address, create socket and try to connect -> if anything fails return -1
    int res = getaddrinfo(host, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr, "[%s] Error: getaddrinfo() (%s)\n", prog_name, gai_strerror(res));
        return -1;
    }

    int socket_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (socket_fd == -1)
    {
        fprintf(stderr, "[%s] Error: socket() (%s)\n", prog_name, strerror(errno));
        return -1;
    }

    if (connect(socket_fd, ai->ai_addr, ai->ai_addrlen) == -1)
    {
        fprintf(stderr, "[%s] Error: connect() (%s)\n", prog_name, strerror(errno));
        return -1;
    }

    freeaddrinfo(ai);
    return socket_fd;
}

/**
 * @brief
 * closes the specified file
 * 
 * @details
 * Closes the specified File only if it is not stdout.
 * @param output_file The FILE* which should be closed.
**/
void safe_close_output_file(FILE* output_file){
    if (output_file != stdout)
    {
        fclose(output_file);
    }
}

/**
 * Program entry point
 * @brief
 * This program servers as a http client (version 1.1)
 * 
 * @details
 * This program servers as a http client (version 1.1). It parses the arguments and sets up the connection accordingly.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE.
**/
int main(int argc, char *argv[])
{
    prog_name = argv[0];

    int opt;
    bool p_flag = false;
    bool o_flag = false;
    bool d_flag = false;
    FILE *output_file = stdout;

    //assign default values
    char *port = "80";
    char *path_opt = "";
    char *url_opt = "";

    while ((opt = getopt(argc, argv, "p:o:d:")) != -1)
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
        case 'o':
            if (o_flag == true)
            {
                usage();
            }
            o_flag = true;

            path_opt = optarg;
            break;
        case 'd':
            if (d_flag == true)
            {
                usage();
            }
            d_flag = true;

            path_opt = optarg;
            break;
        default:
            usage();
        }
    }

    // If -o and -d were specified: Call usage and exit because only one of these options is allowed.
    if (o_flag == true && d_flag == true)
    {
        usage();
    }

    // If no positional argument (=URL) was specified: Call usage and exit. Otherwise copy the argument to url
    if (optind != argc -1)
    {
        usage();
    }
    url_opt = argv[optind];

    char host[strlen(url_opt)];
    char output_path[strlen(url_opt) + strlen(path_opt) + strlen("/index.html")];
    char request_path[strlen(url_opt) + 500];
    

    parse_arguments(path_opt, url_opt, host, output_path, request_path, d_flag);

    fprintf(stderr, "Port=%s output_path=%s host=%s request_path=%s\n", port, output_path, host, request_path);


    if (strcmp(output_path, "") != 0)
    {
        output_file = fopen(output_path, "w");
        if (output_file == NULL)
        {
            fprintf(stderr, "[%s] Error fopen failed (%s)\n", prog_name, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    int socket_fd = set_up_connection(port, host, request_path);
    if(socket_fd == -1){
        fprintf(stderr, "[%s] Connection setup failed.\n", prog_name);
        close(socket_fd);
        safe_close_output_file(output_file);
        exit(EXIT_FAILURE);
    }

    FILE *socket_file = fdopen(socket_fd, "r+");
    if (socket_file == NULL)
    {
        fprintf(stderr, "[%s] Error fopen with socket_file failed\n", prog_name);
        close(socket_fd);
        safe_close_output_file(output_file);
        exit(EXIT_FAILURE);
    }
    
    send_get_request(host, request_path, socket_file);

    // Read and validate header. If value != 0 an error occurred
    int exit_code = read_header_and_validate(socket_file);
    if(exit_code != 0){
        //free resources
        safe_close_output_file(output_file);
        fclose(socket_file);
        exit(exit_code);
    }
    read_content(socket_file, output_file);

    //free resources
    safe_close_output_file(output_file);
    fclose(socket_file);

    exit(EXIT_SUCCESS);
}