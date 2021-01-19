#include "utils.h"
#include "server.h"

static server_arg_t parse_arguments(int argc, char** argv);
static void set_signal_handler(void);
static server_socket_t setup_server_socket(void);
static client_connection_t accept_next_connection(void);
static void handle_connection(client_connection_t conn);
static http_request_t get_request_header(client_connection_t conn);
static http_response_t create_response_header(http_request_t req);
static void send_response_header(client_connection_t conn, http_response_t res);
static void handle_signal(int signal);
static size_t get_file_size(FILE* file);
static char* get_current_date_time(void);
static char* get_mime_type(char* path);
static void close_server_socket(server_socket_t sock);

char* PROGRAM_NAME;
char* USAGE_MESSAGE = "Usage: %s [-p PORT] [-i INDEX] \n";

/**
 * @file client.c
 * @author Michael Huber 11712763
 * @date 15.01.2021
 * @brief OSUE Exercise 3 http
 * @details This server program partially implements version 1.1 of the HTTP. 
 * The server waits for connections from clients and transmits the requested files.
 */

// The following variables are global because they are relevant in the whole context of
// the program.

/** Struct containing the arguments of the program */
static server_arg_t args;

/** Struct containing details of the server socket */
static server_socket_t server_socket;

/** Terminate-Flag - set by the signal handler */
static volatile sig_atomic_t running;

/**
 * @brief Main function handling the program flow
 * @details Uses the global variables args, server_socket, running.
 */ 
int main(int argc, char** argv) {
    PROGRAM_NAME = argv[0];
    LOG("Starting server...");

    args = parse_arguments(argc, argv);

    set_signal_handler();

    server_socket = setup_server_socket();

    running = true;
    while(running) {
        LOG("Waiting for new connection...");
        client_connection_t conn = accept_next_connection();
        if (conn.succesful) {
            LOG("New connection successfully established");
            handle_connection(conn);
            LOG("Closed connection");
        } else {
            continue;
        }
    }
    
    close_server_socket(server_socket);
    LOG("Closed server socket and freed all resources");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Parses the options and arguments and returns them in a struct
 * @details This function terminates the program if the usage of the program is violated
 */ 
static server_arg_t parse_arguments(int argc, char** argv) {
    server_arg_t args = {.index = NULL, .port = NULL, .root = NULL};

    int count_p = 0, count_i = 0;
    int c;  
    while((c = getopt(argc, argv, "p:i:")) != -1 ) {
        switch (c) {
            case 'p':
                args.port = optarg;
                count_p++;
                break;

            case 'i':
                args.index = optarg;
                count_i++;
                break;

            case '?':
                USAGE();
                break;

            default:
                USAGE();
                break;
        }
    }
    // wrong usage
    if (count_p > 1 || count_i > 1) {
        USAGE();
    }
    if (argc == optind || argc > (optind+1)) {
        USAGE();
    }

    // parse port
    if (args.port != NULL) {
        parse_port(args.port);
    } else {
        args.port = DEFAULT_PORT_STRING;
    }

    // index
    if (args.index == NULL) {
        args.index = DEFAULT_FILENAME;
    }

    // parse root path and check if directory exists
    args.root = argv[optind];
    DIR* dir = opendir(args.root);
    if (dir == NULL) {
        ERROR_MSG("Root directory does not exist", NULL);
        USAGE();
    } else {
        closedir(dir);
    }

    return args;
}

/** 
 * @brief Sets signal handler for SIGINT and SIGTERM
 */
static void set_signal_handler(void) {
    // set signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0) {
        ERROR_EXIT("Error setting signal handler", strerror(errno));
    }
}

/**
 * @brief Creates a new server socket listening on the port specified in the program arguments
 * and returns a struct containing the socket fd, the port and the addrinfo struct ai.
 * @details Uses the global variable args.
 */ 
static server_socket_t setup_server_socket(void) {
    server_socket_t sock = {.port_string = args.port};

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, sock.port_string, &hints, &(sock.ai));
    if (res != 0) {
        ERROR_EXIT("Could not get address info", strerror(errno));
    }

    sock.fd = socket(sock.ai->ai_family, sock.ai->ai_socktype, sock.ai->ai_protocol);
    if (sock.fd < 0) {
        ERROR_EXIT("Error creating socket", strerror(errno));
    }

    LOG("Socket successfully created");

    // set option
    int optval = 1;
    setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(sock.fd, sock.ai->ai_addr, sock.ai->ai_addrlen) < 0) {
        ERROR_EXIT("Error binding socket", strerror(errno));
    }

    if (listen(sock.fd, 1)  < 0) {
        ERROR_EXIT("Error while listening for connection", strerror(errno));
    }

    LOG("Socket listening on port %s", sock.port_string);

    return sock;
}

/**
 * @brief Waits for a new incoming connection on the server socket, opens the file associated
 * with the connection socket fd and returns a struct containing the client socket fd, socket
 * file and if the connection was successfully established or not.
 * @details Uses the global variable server_socket
 */ 
static client_connection_t accept_next_connection(void) {
    client_connection_t conn =  { .succesful = false };

    // wait for new connection
    conn.socket_fd = accept(server_socket.fd, NULL, NULL);
    if (conn.socket_fd < 0) {
        if (errno != EINTR) {
            ERROR_LOG("Error while accepting new connection", strerror(errno));
        } else {
            LOG("Accept interrupted by Signal");
        }
        return conn;
    }

    // open socket file
    conn.socket_file = fdopen(conn.socket_fd, "r+");
    if (conn.socket_file == NULL) {
        ERROR_LOG("Error creating sockfile", strerror(errno));
        close(conn.socket_fd);
    }

    // return established connection
    conn.succesful = true;
    return conn;
}

/** 
 * @brief Handles an established connection and communicates via HTTP, 
 * parses the request sent by the client, creates the corresponding
 * response and sends it before closing the connection.
 */
static void handle_connection(client_connection_t conn) {
    http_request_t req = get_request_header(conn);
    if (req.bad == false) LOG("Client sent HTTP %s request for %s", req.method, req.path);
    else LOG("Client sent bad request");

    http_response_t res = create_response_header(req);
    send_response_header(conn, res);
    LOG("Sent response status %ld %s", res.status.code, res.status.detail);

    // only send body if successful
    if (res.status.code == OK) {
        char buf[1];
        while (fread(buf, 1, 1, res.content) == 1) {
            fwrite(buf, 1, 1, conn.socket_file);
        }
        LOG("Sent response body");
    }

    if (req.method != NULL) free(req.method);
    if (req.path != NULL) free(req.path);
    if (res.date_time != NULL) free(res.date_time);
    if (res.content != NULL) fclose(res.content);
    if (conn.socket_file != NULL) fclose(conn.socket_file);
}

/**
 * @brief Gets the request header the connected client sent, parses it
 * for validity and returns a struct containing the requested resource path,
 * the request method and whether the request is malformed (bad).
 */
static http_request_t get_request_header(client_connection_t conn) {
    http_request_t req = {.method = NULL, .path = NULL, .bad = false};

    // get first line of header
    char *line = NULL;
    size_t len, buflen;
    if(getline(&line, &buflen, conn.socket_file) < 0) {
        ERROR_LOG("Error reading request header", strerror(errno));
        return req;
    }

    char *line_p = line;

    char* method = strsep(&line_p, " ");
    req.method = strdup(method);
    if (line_p == NULL) {
        ERROR_LOG("Invalid request header", "No path specified");
        req.bad = true;
    } else {
        char* path = strsep(&line_p, " ");
        req.path = strdup(path);
        if (line_p == NULL) {
            ERROR_LOG("Invalid request header", "No protocol specified");
            req.bad = true;
        } else {
            if (strcmp(line_p, "HTTP/1.1\r\n") != 0) {
                ERROR_LOG("Invalid request header", "Wrong procotol specified");
                req.bad = true;
            }
        }
    }

    // skip rest of header
    while ((len = getline(&line, &buflen, conn.socket_file)) != -1) {
        if (strcmp(line, "\r\n") == 0) break;
    }
    free(line);

    return req;
}

/**
 * @brief Creates a response for the given request - in the case of a
 * valid request opens the requested resource, gets it content type and length,
 * gets the current date/time and returns a struct containing the response values.
 */
static http_response_t create_response_header(http_request_t req) {
    http_response_t res = { .content = NULL, .content_length = 0 };

    // check for errors in request
    if (req.method == NULL) { // 500
        res.status.code = INTERNAL_SERVER_ERROR;
        res.status.detail = INTERNAL_SERVER_ERROR_STRING;
        return res;
    } else if (req.bad == true) { // 400
        res.status.code = BAD_REQUEST;
        res.status.detail = BAD_REQUEST_STRING;
        return res;
    } else if (strcmp(req.method, GET) != 0) { // 501
        res.status.code = NOT_IMPLEMENTED;
        res.status.detail = NOT_IMPLEMENTED_STRING;
        return res;
    }

    // 200 OK

    // add index file to path if request path is root
    char* requested_path;
    if (strcmp(req.path, "/") == 0) {
        requested_path = malloc(strlen(args.index) + 1 + 1);
        if (requested_path == NULL) {
            ERROR_LOG("malloc failed", strerror(errno));
            res.status.code = INTERNAL_SERVER_ERROR;
            res.status.detail = INTERNAL_SERVER_ERROR_STRING;
            return res;
        }
        strcpy(requested_path, req.path);
        strcat(requested_path, args.index);
    } else {
        requested_path = strdup(req.path);
        if (requested_path == NULL) {
            ERROR_LOG("strdup failed", strerror(errno));
            res.status.code = INTERNAL_SERVER_ERROR;
            res.status.detail = INTERNAL_SERVER_ERROR_STRING;
            return res;
        }
    }

    // concat doc_root with path
    char* path = malloc(strlen(args.root) + strlen(requested_path) + 1);
    strcpy(path, args.root); strcat(path, requested_path);
    free(requested_path);

    // check if file exists
    if (access(path, F_OK) != 0) {
        res.status.code = NOT_FOUND;
        res.status.detail = NOT_FOUND_STRING;
        free(path);
        return res;
    }

    // get mime-type of file
    res.mime_type = get_mime_type(path);

    // open file
    res.content = fopen(path, "r");
    free(path);
    if (res.content == NULL) {
        res.status.code = INTERNAL_SERVER_ERROR;
        res.status.detail = INTERNAL_SERVER_ERROR_STRING;
        return res;
    }

    // get other header fields and return
    res.content_length = get_file_size(res.content);
    res.date_time = get_current_date_time();
    res.status.code = OK;
    res.status.detail = OK_STRING;
    return res;
}

/**
 * @brief Sends the response header res to the connected client.
 */
static void send_response_header(client_connection_t conn, http_response_t res) {
    if (res.status.code != OK) {
        fprintf(conn.socket_file, "HTTP/1.1 %ld %s\r\nConnection: close\r\n\r\n", res.status.code, res.status.detail);
    } else if (res.mime_type == NULL) {
        fprintf(conn.socket_file, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", res.date_time, res.content_length);
    } else {
        fprintf(conn.socket_file, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", res.date_time, res.mime_type, res.content_length);
    }
}

/**
 * @brief Closes the server socket sock and frees the addrinfo struct associated with it.
 */
static void close_server_socket(server_socket_t sock) {
    close(sock.fd);
    freeaddrinfo(sock.ai);
}

/**
 * @brief Gets the content type of a file depending on the file extension.
 */
static char* get_mime_type(char* path) {
    char* extension = strrchr(path, '.');
    if (strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0) {
        return "text/html";
    } else if (strcmp(extension, ".css") == 0) {
        return "text/css";
    } else if (strcmp(extension, ".js") == 0) {
        return "application/javascript";
    } else {
        return NULL;
    }
}

/**
 * @brief Gets the size of a file in bytes.
 */
static size_t get_file_size(FILE* file) {
    size_t size;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

/**
 * @brief Gets the current UTC time and returns it in a RFC822-compliant format.
 */
static char* get_current_date_time(void) {
    char* date_time = malloc(256);
    time_t tictoc;
    struct tm *now;
    time(&tictoc);
    now = gmtime(&tictoc);
    if (strftime(date_time, 256, "%a, %d %b %y %T GMT", now) < 0) {
        ERROR_LOG("Error getting current date/time", strerror(errno));
    }
    return date_time;
}


/**
 * @brief Sets the running-variable false, which in consequence terminates the program.
 */
static void handle_signal(int signal) {
    running = false;
}