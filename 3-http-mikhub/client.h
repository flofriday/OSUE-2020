#ifndef _CLIENT_H_
#define _CLIENT_H_

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

#define strempty(s) (s[0] == '\0')

#define DEFAULT_PORT 80
#define DEFAULT_PORT_STRING "80"
#define DEFAULT_FILENAME "index.html"

#define URL_DELIM ";/?:@=&"
#define HEADER_PROTOCOL "HTTP/1.1"

#define PROTOCOL_ERROR "Protocol error!"
#define EXIT_PROTOCOL_ERROR 2
#define EXIT_STATUS_ERROR 3

/**
 * @brief Represents an HTTP GET request
 */ 
typedef struct {
    /** Hostname of the server to send request to */
    char* host;
    /** Port of the server to send request to (as string and as long) */
    char* port_string;
    long port;
    /** Directory path to request */
    char* dirpath;
    /** Filename to save response to */
    char* filename;
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
} http_response_t;

/**
 * @brief Represents a connection via a socket
 */
typedef struct {
    /** Address info for host to connect to */
    struct addrinfo *ai;
    /** File descriptor for socket */
    int socket_fd;
    /** File stream for socket */
    FILE *socket_file;
 } connection_t;

/**
 * @brief Arguments for the client program
 */ 
 typedef struct {
    /** Port to try to connect to */
    char* port;
    /** Filename where response should be stored in */
    char* file; 
    /** Director where response should be stored in */
    char* dir;
 } client_arg_t;

#endif
