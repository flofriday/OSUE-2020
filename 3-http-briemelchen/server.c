/**
 * @author briemelchen
 * @date 03.01.2021
 * @brief Module which represents a HTTP 1.1 server supporting request-method GET.
 * @details The server waits for incoming HTTP-GET requests and transmitts the files
 * asked. The server has the following options:
 * -p specifies the port, where the server should listen to (default 8080)
 * -i specifiees the index-filename of the server which should be
 * sent, if no specific file is requested (default index.html)
 * The positional argument DOC_ROOT specifies the path to the root directory which
 * contain files that can be requested.
 * The server can encode files into gzip, using the in @see gziputils specified routines.
 **/
#include "server.h"

/**
 * @brief setups a socket, so that it is ready to handle requests.
 * @details Creates addrinfo struct which defined used options for the socket-address:
 * hints are: 
 * family is AF_INET and therefore the program uses the IPv4
 * socktype is SOCK_STREAM so a bidirectional connection is made, connection-based (TCP)
 * AI_PASSIVE so that the socket is marked as passiv and can be used for bind.
 * After setting up the struct, the socket sys-call is made; Afterwards binding starts
 * and the listen-sys call is invoked (using a backlog of 5).
 * Finally setsockopt is used, to bypass "Already in Use "error.
 * @param port where the server should listen to
 * @return the socket's file descriptor on success, otherwise -1
 * */
static int setup_socket(char *port);

/**
 * @brief handles ingoing requests and responses appropriate
 * @details Handles as long requests until the singal-handler sets the quit-flag.
 * Blocks for accept-calls and waits till a client wants to connect. Afterwards
 * the request is checked and the response is computed. Finally the response-header
 * is sent, then followed by the response-body.
 * @param sockfd the sockets file descriptor
 * @param doc_root the path to the document's root directory
 * @param index_file the file which should be used if no-other is specifed
 **/
static void accept_and_response(int sockfd, char *doc_root, char *index_file);

/**
 * @brief Checks the first-line of a HTTP-request header on validty and parses it.
 * @details checks the header for the following format:
 * METHOD PATH HTTPVERSION . If  one of it is missing, or additional fields are given,
 * the connection will be closed afterwards.
 * Method and requested path are parsed and returned using pointers to pointers.
 * @param line of the http header
 * @param method pointer to a pointer where the request-method is been stored
 * @param resource_path pointer to a pointer where the path to the resources should be stored
 * @return true on success (valid header),  false otherwise
 **/
static bool get_request(char *line, char **method, char **resource_path);

/**
 * @brief extracts the full path to the requested file
 * @details concats the requested file and the doc_root, so that the full path can be
 * computed and the correct file returned. If no file was specified, the index-file is used.
 * @param doc_root the servers doc-root, where the files are stored 
 * @param requested_file the path to the file requested by the caller
 * @param index_file the default file, in case that the requester has only specified a path
 * @return the full path on success, NULL in case of an error
 **/
static char *get_full_path(char *doc_root, char *requested_file, char *index_file);

/**
 * @brief sends the correct response header to the client.
 * @details the server supports the following status-codes:
 * 400 is sent, if the request header was invalid
 * 404 is sent, if the requested file does not exist.
 * 501 is sent, if the request-method is not supported (any other then GET is not supported)
 * 200 on success
 * All headers contain the "Connection: close" field, so that the server closes the connection if he finishes.
 * On success, a few other content-header are sent:
 * "Date: date" as specified in RFC 822
 * "Content-Length: length" the size of the transmitted file
 * "Content-Encoding: gzip" if the client told the server that it supports it
 * "Content-Type: Mime-Type" is supported only for html/htm, css and js files
 * @param connection_file the file to the socket-connection
 * @param res_code the computed response-code: 200, 400, 404 or 501
 * @param mime_type of the file, NULL if non-supported mime-type
 * @param gzip true,if the client supports gzip, else false
 * @param file_size the size of the file which should be transmitted
 * @return 0 on success, -1 on failure
 * */
static int send_header(FILE *connection_file, int res_code, char *mime_type, bool gzip, int file_size);

/**
 * @brief sends the content of the file which should be transmitted.
 * @details sends the file either as plain-text/bits or as gzip-compressed (@see gziputils.h/c)
 * The writing is performed using fwrite, so binary-data can be sent as well (e.g. JPEG files)
 * @param connection_file the socket's connection-file, where the content should be written to
 * @param requested_file the file requested  by the client
 * @param gzip indicates if sending should be performed using compression (if gzip true), otherwise plain-data will be sent
 * @param size the file size
 * @return 0 on success, -1 on failue
 **/
static int send_content(FILE *connection_file, FILE *req_file, bool gzip, int size);

/**
 * @brief skips the request header and checks if the client supports gzip-encoding
 * @details checking is done by looking for the "Accept-Encoding" field in the header
 * and checks if it contains "gzip"
 * @param connection_file socket's connection file of the current communication
 * @return true, if the client supports encoding, otherwise false
 **/
static bool check_encoding_skip_header(FILE *connection_file);

/**
 * @brief setups the signal-handling
 * @details handled signals are SIGINT and SIGTERM
 * as handler the routine "handle_signal(int signal)" is used.
 **/
static void setup_signal_handler(void);

/**
 * @brief handles signals
 * @details sets the global quit flag, and therefore informs the
 * server to exit.
 * @param signal signal
 **/
static void handle_signal(int signal);

/**
 * @brief prints the usage message to stderr and exits the program
 * @details usage message has format:
 * Usage: PROGR_NAME [-p PORT] [-i INDEX] DOC_ROOT
 * Exit's with exit-code 1
 **/
static void usage(void);

static volatile sig_atomic_t quit; // global quit flag, which is used to stop the program using the signal handler

static char *PROGRAM_NAME; // the program's name

/**
 * @brief starting point of the program: parses options/arguments and calls other functions to handle requests
 * @details parses following options:
 *  -p specifies the port, where the server should listen to (default 8080)
 * -i specifiees the index-filename of the server which should be
 * As positional argument DOC_ROOT the root of the documents has to be specified.
 * Afterwards the arguments and options are checked and routines are called,
 * to setup the socket, signalhandler and start accepting requests.
 * @param argc the argument count containing the number of options and arguments
 * @param argv the argument vector contatining the program-name [0], options and arguments.
 * @return 0 on success, 1 in case of an  error 
 **/
int main(int argc, char *argv[])
{
    PROGRAM_NAME = argv[0];
    quit = false;
    char c;
    char *port = NULL, *index_file = NULL, *doc_root = NULL;
    int p_count = 0, i_count = 0;
    while ((c = getopt(argc, argv, "i:p:")) != -1)
    {
        switch (c)
        {
        case 'p':
            p_count++;
            port = optarg;
            break;
        case 'i':
            i_count++;
            index_file = optarg;
            break;
        default:
            usage();
            break;
        }
    }
    // checking options and arguments
    if (p_count > 1 || i_count > 1)
        usage();
    if (p_count == 0)
        port = DEFAULT_PORT;
    if (!is_valid_port(port))
        usage();
    if (i_count == 0)
        index_file = DEFAULT_FILE;
    if (argv[optind] == NULL)
        usage();
    doc_root = argv[optind];
    setup_signal_handler();
    //set up socket and start accepting requests
    int sockfd;
    if ((sockfd = setup_socket(port)) == -1)
        error("Failed to setup socket!", strerror(errno), PROGRAM_NAME);
    accept_and_response(sockfd, doc_root, index_file);
}

static void accept_and_response(int sockfd, char *doc_root, char *index_file)
{
    while (!quit)
    {
        int connfd;
        if ((connfd = accept(sockfd, NULL, NULL)) < 0)
        {
            if (errno == EINTR) // signal, check loop-condition(may has been set) otherwise try to call accept again
                continue;

            error("accept failed!", strerror(errno), PROGRAM_NAME);
        }

        int res_code = 200;

        // file used for the connection r+ because writing is needed aswell
        FILE *connection = fdopen(connfd, "r+");
        if (connection == NULL)
        {
            error("fdopen failed!", strerror(errno), PROGRAM_NAME);
        }

        // checking and parsing request header
        char *line = NULL;
        size_t len = 0;
        ssize_t nread;
        char *request_method = NULL;
        char *resource_path = NULL;
        char *dup = NULL;
        if ((nread = getline(&line, &len, connection)) != -1)
        {
            dup = strdup(line);
            if (!get_request(dup, &request_method, &resource_path))
            {
                res_code = 400;
            }
        }
        free(line);

        // skip header and check if encoding is  desired
        bool gzip = check_encoding_skip_header(connection);

        if (request_method == NULL || resource_path == NULL)
        {
            if (fclose(connection) < 0)
            {
                error("Fclose failed", strerror(errno), PROGRAM_NAME);
            }
            continue;
        }

        if (strcmp(request_method, "GET") != 0 && res_code == 200) // non GET method is requested
            res_code = 501;

        char *full_file_path;
        if ((full_file_path = get_full_path(doc_root, resource_path, index_file)) == NULL)
        {
            error("Extracting full path failed!", strerror(errno), PROGRAM_NAME);
        }
        FILE *req_file = NULL;
        if (res_code == 200)
        {
            req_file = fopen(full_file_path, "r");
            if (req_file == NULL)
            {
                if ((errno == ENOENT || errno == ENOTDIR)) // File not found
                    res_code = 404;
                else
                    error("Failed to open file!", strerror(errno), PROGRAM_NAME);
            }
        }

        int content_size = 0;
        if (res_code == 200)
        {
            // get content size either encoded-size or plain-size
            if (gzip)
            {
                if (compress_gzip(req_file, NULL, &content_size) < 0)
                {
                    error("Error while deflating using zlib!", strerror(errno), PROGRAM_NAME);
                }
            }
            else
            {
                if ((content_size = get_file_size(req_file)) < 0)
                {
                    error("Error while getting filesize!", strerror(errno), PROGRAM_NAME);
                }
            }
        }

        // sending header
        if (send_header(connection, res_code, get_mime_type(full_file_path), gzip, content_size) == -1)
        {
            error("Failed to send header", strerror(errno), PROGRAM_NAME);
        }

        if (res_code == 200)
        {
            // send content if and only if 200 is used as response code
            if (send_content(connection, req_file, gzip, content_size) < 0)
                error("Failed to send content!", "", PROGRAM_NAME);
        }

        printf("REQUEST-METHOD:%s, REQUESTED-FILE:%s, RESPONSE-CODE:%d, ENCODED: %s\n",
               request_method, full_file_path, res_code, gzip ? "Y" : "N");
        fflush(stdout);
        if (fclose(connection) < 0)
            error("fclose failed!", strerror(errno), PROGRAM_NAME);
        if (req_file != NULL)
        {
            if (fclose(req_file) < 0)
                error("fclose failed!", strerror(errno), PROGRAM_NAME);
        }

        free(full_file_path);
        free(dup);
    }
}

static bool check_encoding_skip_header(FILE *connection_file)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    int re_value = false;
    while ((nread = getline(&line, &len, connection_file)) != -1 && (strcmp(line, "\r\n") != 0))
    {
        if (strncmp(line, "Accept-Encoding", 15) == 0)
        {
            if (strstr(line, "gzip") != NULL)
                re_value = true;
        }
    }
    free(line);
    return re_value;
}

static int send_content(FILE *connection_file, FILE *req_file, bool gzip, int size)
{

    if (gzip)
    {
        compress_gzip(req_file, connection_file, &size);
    }
    else
    {
        char buffer[size];
        memset(buffer, 0, size);
        int res = fread(buffer, 1, size, req_file);
        if (res < 0)
            return -1;
        buffer[res] = '\0';
        if (fwrite(buffer, 1, res, connection_file) != res)
            return -1;
    }
    if (ferror(req_file) || ferror(connection_file))
        return -1;
    return 0;
}

static int send_header(FILE *connection_file, int res_code, char *mime_type, bool gzip, int file_size)
{
    char date[256];
    time_t t;
    struct tm *tmp;
    time(&t);
    tmp = gmtime(&t);
    if (strftime(date, sizeof(date), "%a, %d %b %g %T GMT", tmp) == 0)
        return -1;

    if (date == NULL || file_size == -1)
        return -1;

    switch (res_code)
    {
    case 200:
        fprintf(connection_file, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %d\r\nConnection: close\r\n", date, file_size);
        if (mime_type != NULL)
            fprintf(connection_file, "Content-Type: %s\r\n", mime_type);
        if (gzip)
            fprintf(connection_file, "Content-Encoding: gzip\r\n");
        fprintf(connection_file, "\r\n");
        break;
    case 400:
        fprintf(connection_file, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        break;
    case 404:
        fprintf(connection_file, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
        break;
    case 501:
        fprintf(connection_file, "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n");
        break;
    default:
        return -1;
        break;
    }
    fflush(connection_file);
    return 0;
}

static char *get_full_path(char *doc_root, char *requested_file, char *index_file)
{
    char *full = malloc(sizeof(char) * (strlen(doc_root) + strlen(requested_file) + strlen(index_file) + 1));
    if (full == NULL)
        return NULL;
    full[0] = '\0';
    strcpy(full, doc_root);
    strcat(full, requested_file);
    if (requested_file[strlen(requested_file) - 1] == '/')
    {
        strcat(full, index_file);
    }
    return full;
}

static bool get_request(char *line, char **method, char **resource_path)
{
    *method = strtok(line, " ");
    *resource_path = strtok(NULL, " ");
    char *http_v = strtok(NULL, " ");
    if (*method == NULL || *resource_path == NULL || http_v == NULL)
    {
        printf("HERE");
        return false;
    }
    if (strncmp(http_v, "HTTP/1.1", strlen("HTTP/1.1")) != 0)
        return false;

    if (strtok(NULL, " ") != NULL) // additional values given -> malformed request!
        return false;

    return true;
}

static int setup_socket(char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &ai);
    if (res != 0)
    {
        freeaddrinfo(ai);
        return -1;
    }
    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd < 0)
    {
        freeaddrinfo(ai);
        return -1;
    }
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0)
    {
        freeaddrinfo(ai);
        return -1;
    }
    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        freeaddrinfo(ai);
        return -1;
    }
    if (listen(sockfd, 5) != 0)
    {
        freeaddrinfo(ai);
        return -1;
    }

    freeaddrinfo(ai);
    return sockfd;
}

static void setup_signal_handler(void)
{
    struct sigaction sig_handler;
    memset(&sig_handler, 0, sizeof(sig_handler));
    sig_handler.sa_handler = handle_signal;

    sigaction(SIGINT, &sig_handler, NULL);
    sigaction(SIGTERM, &sig_handler, NULL);
}

static void handle_signal(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
        quit = 1;
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s [-p PORT] [-i INDEX] DOC_ROOT \n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}