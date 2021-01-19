#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>

#define DEFAULT_PORT 8080
#define DEFAULT_PORT_STRING "8080"
#define DEFAULT_FILENAME "index.html"

#define OK 200
#define OK_STRING "OK"
#define NOT_IMPLEMENTED 501
#define NOT_IMPLEMENTED_STRING "Not implemented"
#define NOT_FOUND 404
#define NOT_FOUND_STRING "Not Found"
#define BAD_REQUEST 400
#define BAD_REQUEST_STRING "Bad Request"
#define INTERNAL_SERVER_ERROR 500
#define INTERNAL_SERVER_ERROR_STRING "Internal Server Error"

#define GET "GET"

/**
 * @brief Represents an HTTP request
 */
typedef struct {
    /** HTTP method used in request */  
    char* method;
    /** Requested file path */
    char* path;
    /** Whether the request is malformed */
    bool bad;
} http_request_t;

/**
 * @brief Represents an HTTP response status
 */
typedef struct {
    /** HTTP status code (e.g. 200) */
    long code;
    /** HTTP status detail (e.g. "OK") */
    char* detail;
} http_status_t;

/**
 * @brief Represents an HTTP response
 */
typedef struct {
    /** Status of HTTP response */
    http_status_t status;
    /** Date and time the response was sent */
    char* date_time;
    /** Content type of the file in the response body */
    char* mime_type;
    /** Content length of the file in the response body */
    size_t content_length;
    /** File to be sent in response body */
    FILE* content;
} http_response_t;

/**
 * @brief Represents a connected client
 */
typedef struct {
    /** File descriptor of the client socket */
    int socket_fd;
    /** File stream of the client socket */
    FILE *socket_file;
    /** Whether the connection was successfully established */
    bool succesful;
 } client_connection_t;

 typedef struct {
     /* Port to start server on */
     char* port_string;
     /* Address info for server socket */
     struct addrinfo *ai;
     /* File descriptor of server socket */
     int fd;
 } server_socket_t;

 /**
 * @brief Arguments for the server program
 */ 
 typedef struct {
    /** Port to start server on */
    char* port;
    /** File to respond with if no path is specified in request */
    char* index; 
    /** Directory to serve files out of */
    char* root;
 } server_arg_t;

#endif