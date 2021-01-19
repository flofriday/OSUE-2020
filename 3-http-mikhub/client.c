#include "utils.h"
#include "client.h"

static void initialize(void);
static client_arg_t parse_arguments(int argc, char** argv);
static http_request_t parse_url(char* url_arg);
static FILE* open_output_file(client_arg_t arg);
static void connect_to_address(http_request_t req);
static void send_request(http_request_t req);
static http_status_t get_response_header(void);
static void get_and_write_response_body(void);
static void cleanup(void);

char* PROGRAM_NAME;
char* USAGE_MESSAGE = "Usage: %s [-p PORT] [ -o FILE | -d DIR ] URL\n";

/**
 * @file client.c
 * @author Michael Huber 11712763
 * @date 15.01.2021
 * @brief OSUE Exercise 3 http
 * @details This program partially implements version 1.1 of the HTTP. 
 * The client takes an URL as input, connects to the corresponding server 
 * and requests the file specified in the URL. The transmitted content of 
 * that file is written to stdout or to a file.
 */

// The following variables are global because they are relevant in the whole context of
// the program and it makes a clean exit easier because I can close files and free
// memory in a central exit function.

/** Struct containing the variables regarding the socket connection */
static connection_t conn;

/** Stream to write response body to */
static FILE* out;

/** Struct containing details of HTTP request to be made */
static http_request_t req;
/** Struct containing status code of HTTP response */
static http_response_t res;

/**
 * @brief Main function handling the program flow
 * @details Uses the global variables out, req, res, conn
 */
int main(int argc, char** argv) {
    PROGRAM_NAME = argv[0];

    initialize();

    client_arg_t args = parse_arguments(argc, argv);

    out = open_output_file(args);
    
    connect_to_address(req);

    send_request(req);

    res.status = get_response_header();
    if (res.status.code == -1) {
        fprintf(stderr, "%s\n", PROTOCOL_ERROR);
        exit(EXIT_PROTOCOL_ERROR);
    } else if (res.status.code != 200) {
        fprintf(stderr, "%ld %s\n", res.status.code, res.status.detail);
        exit(EXIT_STATUS_ERROR);
    }

    get_and_write_response_body();

    // cleanup is done via exit-function
    exit(EXIT_SUCCESS);
}

/**
 * @brief Initializes global structs and defines exit function
 * @details Uses global variables out, conn, res
 */ 
static void initialize(void) {
    // initialize global structs which have to be freed if assigned 
    // (NULL is assigned so it can determined whether memory needs to be freed)
    out = NULL;
    conn.ai = NULL;
    conn.socket_file = NULL;
    res.status.detail = NULL;

    // register exit function
    if(atexit(cleanup) < 0) {
        ERROR_EXIT("Could not register exit function", strerror(errno));
    }
}

/**
 * @brief Parses the options and arguments, fills the global struct req with the argument 
 * and returns the options in a struct
 * @details Uses the global variables req and fills it; This function terminates the program
 * if the usage of the program is violated
 */ 
static client_arg_t parse_arguments(int argc, char** argv){
    client_arg_t args = {.dir = NULL, .file = NULL, .port = NULL};

    // parse options
    int count_p = 0, count_o = 0, count_d = 0;
    int c;  
    while((c = getopt(argc, argv, "p:o:d:")) != -1 ) {
        switch (c) {
            case 'p':
                args.port = optarg;
                count_p++;
                break;

            case 'o':
                args.file = optarg;
                count_o++;
                break;

            case 'd':
                args.dir = optarg;
                count_d++;
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
    if (count_p > 1 || count_o > 1 || count_d > 1 || (count_o > 0 && count_d > 0)) {
        USAGE();
    }
    if (argc == optind || argc > (optind+1)) {
        USAGE();
    }

    // parse requested URL from args
    req = parse_url(argv[optind]);
    if (req.filename == NULL) {
        req.filename = DEFAULT_FILENAME;
    }

    // parse port
    if (args.port != NULL) {
        req.port = parse_port(args.port);
        req.port_string = args.port;
    } else {
        req.port = DEFAULT_PORT;
        req.port_string = DEFAULT_PORT_STRING;
    }

    return args;
}

/**
 * @brief Takes a URL, splits it into its part and returns it as a http_request_t struct
 */ 
static http_request_t parse_url(char* url_arg) {
    if (strncmp(url_arg, "http://", 7) != 0) {
        fprintf(stderr, "[%s]: Invalid URL '%s' (has to start with 'http://')\n", PROGRAM_NAME, url_arg);
        USAGE();
    }

    http_request_t req;

    char* url_p = &(url_arg[7]);

    // get host (part until URL_DELIM)
    req.host = strsep(&url_p, URL_DELIM);

    // get path (part after URL_DELIM) and filename (after last '/')
    if (url_p == NULL) { // url of form xyz.com (without '/')
        req.dirpath = NULL;
        req.filename = NULL;
    } else if (strempty(url_p) == true) { // url of form xyz.com/
        req.dirpath = url_p;
        req.filename = NULL;
    } else { // url of form xyz.com/...
        req.dirpath = url_p;
        req.filename = strrchr(url_p, '/'); // gets the part of ther url after last '/'

        if (req.filename == NULL) { // url of form xyz.com/abc (no '/' found, filename == dirpath)
            req.filename = req.dirpath;
        } else { // url of form xyz.com/abc/[...]
            req.filename = req.filename+1; // remove '/' from filename
             if (strempty(req.filename) == true) { // url of form xyz.com/abc/ (directory requested, thus no filename specified)
                req.filename = NULL;
             }
        }
    } 
    
    return req;
}

/**
 * @brief Opens an output file if specified in the args struct, otherwise returns stdout
 * @details If the -o option was specified, a file with the given argument as filename is
 * opened/created; If the -d option was specified, a file with filename given in the request
 * url is created in the directory specified by the option.
 */
static FILE* open_output_file(client_arg_t arg) {
    FILE* outfile;
    if (arg.file!= NULL) {
        outfile = fopen(arg.file, "w");
        if (outfile == NULL) {
            ERROR_EXIT("Could not create output file", strerror(errno));
        }
    } else if (arg.dir != NULL) {
        char path[strlen(arg.dir) + strlen(req.filename) + 1];
        strcpy(path, arg.dir); strcat(path, "/"); strcat(path, req.filename);
        outfile = fopen(path, "w");
        if (outfile == NULL) {
            ERROR_EXIT("Could not create output directory/file", strerror(errno));
        }
    } else {
        outfile = stdout;
    }
    return outfile;
}

/**
 * @brief Connects to the address specified in the argument using a socket
 * @details Uses and fills the global variable conn
 */ 
static void connect_to_address(http_request_t req) {
    // connection
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(req.host, req.port_string, &hints, &(conn.ai));
    if (res != 0) {
        ERROR_EXIT("Could not get address info", (char*)gai_strerror(res));
    }

    conn.socket_fd = socket(conn.ai->ai_family, conn.ai->ai_socktype, conn.ai->ai_protocol);
    if (conn.socket_fd < 0) {
        ERROR_EXIT("Error creating socket", strerror(errno));
    }

    if (connect(conn.socket_fd, conn.ai->ai_addr, conn.ai->ai_addrlen) < 0) {
        ERROR_EXIT("Error connecting to socket", strerror(errno));
    }

    conn.socket_file = fdopen(conn.socket_fd, "r+");
    if (conn.socket_file == NULL) {
        ERROR_EXIT("Error creating sockfile", strerror(errno));
    }
}

/** 
 * @brief Sends the request given in the argument to the connected socket
 * @details Uses the global variable conn
 */
static void send_request(http_request_t req) {
    if(fprintf(conn.socket_file, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection:close\r\n\r\n", req.dirpath, req.host) < 0) {
        ERROR_EXIT("Error while writing request", strerror(errno));
    }
    if(fflush(conn.socket_file) == EOF) {
        ERROR_EXIT("Error while flushing request", strerror(errno));
    }
}

/**
 * @brief Reads and parses the response from the connected socket and returns the 
 * status code (if present)
 * @details Uses the global variable conn
 */
static http_status_t get_response_header(void) {
    http_status_t status = { .code = 0, .detail = NULL };

    // get first line of header
    char* line = NULL;
    size_t len, buflen;
    if(getline(&line, &buflen, conn.socket_file) < 0) {
        ERROR_EXIT("Error reading response header", strerror(errno));
    }

    // check if header starts with right protocol
    if (strncmp(line, HEADER_PROTOCOL, strlen(HEADER_PROTOCOL)) != 0) {
        free(line);
        status.code = -1;
        return status;
    }

    // check for status code 
    char *line_status = &(line[strlen(HEADER_PROTOCOL)+1]);
    char *endptr;
    status.code = strtol(line_status, &endptr, 10);
    if (endptr == line_status || status.code == LONG_MIN || status.code == LONG_MAX || status.code < 0) {
        free(line);
        status.code = -1;
        return status;
    }

    // store status detail without /r/n
    size_t detail_length = strlen(endptr+1)-1;
    status.detail = malloc(detail_length);
    strncpy(status.detail, endptr+1, detail_length-1);
    status.detail[detail_length-1] = '\0';

    // skip rest of header
    while ((len = getline(&line, &buflen, conn.socket_file)) != -1) {
        if (strcmp(line, "\r\n") == 0) break;
    }
    free(line);

    return status;
}

/**
 * @brief Reads the response body from the connected socket and writes it to the outfile
 * @details Uses the global variables conn and out
 */ 
static void get_and_write_response_body(void) {
    // read response and write to out-file
    char buf[1];
    while (fread(buf, 1, 1, conn.socket_file) == 1) {
        fwrite(buf, 1, 1, out);
    }
}

/**
 * @brief Cleanup function, closes and frees resources (if they were opened)
 * @brief Uses and closes/frees global variables out, conn, res
 */
static void cleanup(void) {
    if (out != NULL) fclose(out);
    if (conn.socket_file != NULL) fclose(conn.socket_file);
    if (conn.ai != NULL) freeaddrinfo(conn.ai);
    if (res.status.detail != NULL) free(res.status.detail); 
}