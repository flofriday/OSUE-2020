/**
 * @file server.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 19.12.2020
 * 
 * @brief The server program.
 * 
 * Implementation of a simple HTTP server with just the use of the C standard 
 * library.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <zlib.h>
#include <fcntl.h>

/**
 * Internal buffer size.
 * @brief This size is used to create buffers when reading from a file.
 **/
#define BUFFER_SIZE 1024

/**
 * The name of the current program.
 */
char *prog_name;

/**
 * @brief Mostly used to handle signals so that the server can gracefully 
 * shutdown.
 **/
volatile sig_atomic_t alive = true;

/**
 * Signal handler.
 * @brief A simple signal handler.
 * @details Sets the global variable alive.
 * The signal passed to it will be ignored.
 * @param signal The signal to handle.
 */
static void handle_signal(int signal)
{
    alive = false;
}

/**
 * Print usage.
 * @brief Print the usage of the server.
 * @details Reads the global variable prog_name.
 */
static void usage(void)
{
    fprintf(stderr, "[%s] server [-p PORT] [-i INDEX] DOC_ROOT\n", prog_name);
}

/**
 * Read a file into memmory.
 * @brief The input file will be read into memory.
 * @details Data can be NULL pointer as this function will allocate the needed 
 * memory anyway.
 * The caller must free the data buffer after use.
 * @param input The file to be read.
 * @param data The byte buffer to which the files content is written.
 * @return Upon success the number of bytes read otherwise -1.
 */
static size_t read_file(FILE *input, uint8_t **data)
{
    size_t len = 0;
    size_t cap = BUFFER_SIZE;
    *data = malloc(sizeof(uint8_t) * cap);
    if (*data == NULL)
    {
        return -1;
    }

    while (!feof(input))
    {
        if (len + BUFFER_SIZE >= cap)
        {
            cap *= 2;
            uint8_t *tmp = realloc(*data, cap);
            if (tmp == NULL)
            {
                free(*data);
                return -1;
            }
            *data = tmp;
        }

        len += fread(*data + len, sizeof(uint8_t), BUFFER_SIZE, input);
    }
    return len;
}

/**
 * Read a file gzip compressed into memmory.
 * @brief The input file will be read into memmory and compressed with the 
 * gzip algorithm.
 * @details Data can be NULL pointer as this function will allocate the needed 
 * memory anyway.
 * The caller must free the data buffer after use.
 * May use the global variable prog_name.
 * @param input The file to be read.
 * @param data The byte buffer to which the files content is written.
 * @return The number of bytes after the compression.
 * @return Upon success the number of bytes read after the compression
 * otherwise -1.
 */
static size_t compress_file(FILE *input, uint8_t **data)
{
    // Read the file
    uint8_t *raw;
    size_t raw_len = read_file(input, &raw);

    size_t data_len = sizeof(uint8_t) * compressBound(raw_len) + 128;
    *data = malloc(data_len);
    if (*data == NULL)
    {
        free(raw);
        return -1;
    }

    // Setup zlib for using gzip
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    int err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS,
                           8, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
    {
        fprintf(stderr, "[%s] ERROR: Compression Init returned: %d\n",
                prog_name, err);
        free(raw);
        return 0;
    }

    // Compress the data
    stream.next_in = raw;
    stream.avail_in = raw_len;
    stream.next_out = *data;
    stream.avail_out = data_len;
    err = deflate(&stream, true);
    if (err != Z_OK)
    {
        fprintf(stderr, "[%s] ERROR: Compression returned: %d\n", prog_name,
                err);
        free(raw);
        return 0;
    }

    deflateEnd(&stream);
    free(raw);

    return data_len - stream.avail_out;
}

/**
 * Create a socket.
 * @brief Create socket to call accept upon.
 * @details May use the global variable prog_name.
 * @param port The port as a string.
 * @return Upon success a filedescriptor is returned, on error a negative 
 * integer will be returned.
 */
static int create_socket(char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int ai_err = getaddrinfo(NULL, port, &hints, &ai);
    if (ai_err != 0)
    {
        fprintf(stderr, "[%s] ERROR: Unable to get addr info: %s\n",
                prog_name, gai_strerror(ai_err));
        return -1;
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd == -1)
    {
        fprintf(stderr, "[%s] ERROR: Unable to create the socket: %s\n",
                prog_name, strerror(errno));
        freeaddrinfo(ai);
        return -1;
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        fprintf(stderr, "[%s] ERROR: Unable to bind the socket: %s\n",
                prog_name, strerror(errno));
        freeaddrinfo(ai);
        return -1;
    }

    if (listen(sockfd, 32) == -1)
    {
        fprintf(stderr, "[%s] ERROR: Unable to listen to the socket: %s\n",
                prog_name, strerror(errno));
        freeaddrinfo(ai);
        return -1;
    }

    freeaddrinfo(ai);
    return sockfd;
}

/**
 * Write a error header.
 * @brief Writes a HTTP/1.1 minimal header to a file, used for writing error responses.
 * @param conn_file The file to write to.
 * @param status A status text like "400 Bad Request".
 */
static void write_error_header(FILE *conn_file, char *status)
{
    fprintf(conn_file, "HTTP/1.1 %s\r\nConnection: close\r\n\r\n", status);
    fflush(conn_file);
}

/**
 * Write a success heeader.
 * @brief Writes a HTTP/1.1 header to a file provided. This function implements #
 * multiple header fileds.
 * @param conn_file The file to write to.
 * @param filename The name of the file to be served, used to set the
 * Content-Type header.
 * @param filesize The size in bytes of the payload.
 * @param compress A flag to indicate if the content following this header will 
 * be gzip compressed.
 */
static void write_success_header(FILE *conn_file, char *filename, size_t filesize, bool compress)
{
    // Find out the time
    char time_text[200];
    time_t t = time(NULL);
    struct tm *tmp;
    tmp = gmtime(&t);
    strftime(time_text, sizeof(time_text), "%a, %d %b %y %T %Z", tmp);

    fprintf(conn_file,
            "\
HTTP/1.1 200 OK\r\n\
Date: %s\r\n\
Content-Length: %lu\r\n\
Connection: close\r\n",
            time_text, filesize);

    // Find out the contenttype
    char *extention = strrchr(filename, '.');
    if (strcmp(extention, ".html") == 0 || strcmp(extention, ".htm") == 0)
    {
        fprintf(conn_file, "Content-Type: text/html\r\n");
    }
    else if (strcmp(extention, ".css") == 0)
    {
        fprintf(conn_file, "Content-Type: text/css\r\n");
    }
    else if (strcmp(extention, ".js") == 0)
    {
        fprintf(conn_file, "Content-Type: application/javascript\r\n");
    }

    // Tell if we compress
    if (compress)
    {
        fprintf(conn_file, "Content-Encoding: gzip\r\n");
    }

    // End the header
    fprintf(conn_file, "\r\n");
}

/**
 * Handle an incomming request.
 * @brief Handle an incomming request and answer it with a reasonable response.
 * @details Will write log messages to stderr.
 * May use the global variable prog_name. 
 * @param connfd The connection file descriptor.
 * @param index The name of the default file in a directory if no file is 
 * descriped in the request.
 * @param doc_root The name of the folder from which this server will server 
 * files from. 
 * @return 
 */
static int handle_request(int connfd, char *index, char *doc_root)
{
    // Convert the file descriptor to a FILE struct
    FILE *conn_file = fdopen(connfd, "r+");
    if (conn_file == NULL)
    {
        fprintf(stderr, "[%s] ERROR: Unable to fdopen: %s\n",
                prog_name, strerror(errno));
        close(connfd);
        return -1;
    }
    setvbuf(conn_file, NULL, _IONBF, 0);

    // NOTE: The server neeeds to read the whole request before writing any
    // content.

    // parse the first line
    char *first = NULL;
    size_t first_cap = 0;
    if (getline(&first, &first_cap, conn_file) == -1)
    {
        fprintf(stderr, "[%s] Request: 400 Bad Request (No first line)\n", prog_name);
        write_error_header(conn_file, "400 Bad Request");
        free(first);
        fclose(conn_file);
        return 0;
    }
    char *method = strtok(first, " ");
    char *resource = strtok(NULL, " ");
    char *protocol = strtok(NULL, "\r");

    // read all other headerfields
    char *line = NULL;
    size_t cap = 0;
    bool compress = false;
    while (true)
    {
        if (getline(&line, &cap, conn_file) == -1)
        {
            fprintf(stderr, "[%s] Request: 400 Bad Request (No empty line)\n",
                    prog_name);
            write_error_header(conn_file, "400 Bad Request");
            free(first);
            free(line);
            fclose(conn_file);
            return 0;
        }

        if (strcmp(line, "\r\n") == 0)
        {
            break;
        }

        if (strncmp(line, "Accept-Encoding", strlen("Accept-Encoding")) == 0 &&
            strstr(line, "gzip") != NULL)
        {
            compress = true;
        }
    }

    // check if the request is valid (and implemented by this server)
    if (method == NULL || resource == NULL ||
        protocol == NULL || strcmp(protocol, "HTTP/1.1") != 0)
    {
        fprintf(stderr, "[%s] Request: 400 Bad Request (First line: %s %s %s)\n",
                prog_name, method, resource, protocol);
        write_error_header(conn_file, "400 Bad Request");
        free(first);
        free(line);
        fclose(conn_file);
        return 0;
    }

    if (strcmp(method, "GET") != 0)
    {
        fprintf(stderr, "[%s] Request: 501 Not implemented (Method: %s)\n",
                prog_name, method);
        write_error_header(conn_file, "501 Not implemented");
        free(first);
        free(line);
        fclose(conn_file);
        return 0;
    }

    // create the file path
    char filename[strlen(doc_root) + strlen(index) + strlen(resource)];
    strcpy(filename, doc_root);
    strcat(filename, resource);
    if (resource[strlen(resource) - 1] == '/')
    {
        strcat(filename, index);
    }

    // send the header
    FILE *in_file = fopen(filename, "r");
    if (in_file == NULL)
    {
        fprintf(stderr, "[%s] Request: 404 Not Found (File: %s)\n",
                prog_name, filename);
        write_error_header(conn_file, "404 Not Found");
        free(first);
        free(line);
        fclose(conn_file);
        return 0;
    }

    uint8_t *data;
    size_t filesize;

    if (compress)
    {
        filesize = compress_file(in_file, &data);
    }
    else
    {
        filesize = read_file(in_file, &data);
    }

    if (filesize < 0)
    {
        fprintf(stderr, "[%s] Request: 500 Internal Server Error (Ran out of memmory! File: %s) \n",
                prog_name, filename);
        write_error_header(conn_file, "500 Internal Server Error");
        free(first);
        free(line);
        fclose(conn_file);
        return 0;
    }

    fprintf(stderr, "[%s] Request: 200 OK (File: %s)\n",
            prog_name, filename);
    write_success_header(conn_file, filename, filesize, compress);

    // Send the payload
    fwrite(data, sizeof(uint8_t), filesize, conn_file);

    // Cleanup
    free(data);
    free(first);
    free(line);
    fclose(conn_file);
    fclose(in_file);
    return 0;
}

/**
 * The entrypoint of the server.
 * @brief Execution of the server starts and always ends here.
 * @details Uses the global variable prog_name.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS upon success, otherwiese EXIT_FAILURE.
 */
int main(int argc, char *argv[])
{
    prog_name = argv[0];

    // Setup the singal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Parse arguments
    char *port = NULL;
    char *index = NULL;
    int c;
    while ((c = getopt(argc, argv, "p:i:")) != -1)
    {
        switch (c)
        {
        case 'p':
            if (port != NULL)
            {
                usage();
                exit(EXIT_FAILURE);
            }
            port = optarg;
            break;
        case 'i':
            if (index != NULL)
            {
                usage();
                exit(EXIT_FAILURE);
            }
            index = optarg;
            break;
        default:
            assert(false);
            break;
        }
    }

    // Check if there is the DOC_ROOT argument
    if (optind != argc - 1)
    {
        usage();
        exit(EXIT_FAILURE);
    }
    char *doc_root = argv[optind];

    // Set port and index to defaults if not done via arguments
    if (port == NULL)
    {
        port = "8080";
    }
    if (index == NULL)
    {
        index = "index.html";
    }

    // Create the socket
    int sockfd = create_socket(port);
    if (sockfd == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Wait for connections
    int ret = EXIT_SUCCESS;
    while (alive)
    {
        // Wait for a request and receive that socket
        int connfd = accept(sockfd, NULL, NULL);
        if (connfd == -1)
        {
            if (errno != EINTR)
            {
                fprintf(stderr, "[%s] ERROR: Unable to connect: %s\n",
                        prog_name, strerror(errno));
                ret = EXIT_FAILURE;
            }

            alive = false;
            break;
        }

        // Handle the request
        int err = handle_request(connfd, index, doc_root);
        if (err == -1)
        {
            alive = false;
            ret = EXIT_FAILURE;
        }
    }

    // free resources
    close(sockfd);
    return ret;
}
