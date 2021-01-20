/**
 * @author briemelchen
 * @date 03.01.2021
 * @brief HTTP Client, which supports GET requests  HTTP version 1.1
 * @details Implemented using the C socket-API. Only HTTP is supported, no HTTPS so no secure layer.
 *  The Programm allows the following options:
 * - p specifies the port where the client tries to connect to the server 
 *      (without this option uses standard port 80)
 * either - o specifies the outputfile
 * or - d specifies a directory, where a file with the same name as requested file is created and the 
 *      content is written to it. e.g -d ../test/ and GET /test.html; a file test.html 
 *      in ../test/ will be created with the content which was handed by the server
 * If neither o nor d is sepcified, the received content is written to stdout.
 * Along with the mentioned options, the program needs a given URL, where the HTTP-GET request should be performed.
 * Headers supported by the client:
 * Accept-Encoding: gzip -> indicates the Client understands gzip encoding
 * Connection: close -> Connection should be closed after transmitting
 **/
#include "client.h"

/**
 * @brief setup's a connection using C's socket-api
 * @details first setups a addrinfo-struct using SOCK_STREAM to define 
 * a bidirectional conversation (connection-based) and AF_INET to use IPv4 .
 * Then creats a socket using the socket-sys-call.
 * Afterwards initates a connection to the (remote) host using connect() system call.
 * @param host the hosts address e.g. test.com
 * @param port to connect to (default 80)
 * @param sockfd  where the file descriptor of the socket points to, so it can be used later.
 * @return 0 on success, < 0 in case of an error.
 **/
static int setup_socket(char *host, char *port, int *sockfd);

/**
 * @brief reads response from socket and writes it to an out-file.
 * @details reading/writing is performed by fread/fwrite so binary data can be processed aswell.
 *          If the server can encode, the file is decoded by the client (@see gziputil.h), otherwise plain data is read.
 * @param out File to which should be written (specified by options or stdout)
 * @param sockfile file which is associated with the sockets file-descriptor
 * @param encoded true, if the server sends encoded content, otherwise false.
 * @return 0 on success, -1 on failure.
 **/
static int read_response_write(FILE *out, FILE *sockfile, bool encoded);

/**
 * @brief cleanup function to cleanup resources.
 * @details free's all used pointer and closes unclosed files.
 * @param path to be freed
 * @param host to be freed
 * @param dpath to be freed
 * @param sockfile to be closed
 * @param out to be closed
 **/
static void cleanup(char *path, char *host, char *dpath, FILE *sockfile, FILE *out);

/**
 * @brief writes the program's usage message and quits the program.
 * @details the usage message is written to stderr.
 * format: Usage: PROGRAM_NAME [-p PORT] [-o FILE | -d DIR] URL
 * The program exits with exit-code 1.
 **/
static void usage(void);

static char *PROGRAM_NAME; // argv[0] the program's name.

/**
 * @brief starting point of the program, parses arguements and handles the connection.
 * @details program starts with argument parsing from command line. Valid arguments and options are:
 * -p as a number specifing the port, which should be used at the server-side (default 80)
 * either  -o specifies a output file, where the transmitted file should be saved (if no specifed, stdout is used)
 * or -d specifies a directory, where a file with the same name as requested file is created and the 
 *      content is written to it. 
 * URL as positional argument, specifing the URL for the request (has to start with http://)
 * Afterwards the options are checked and handled, if something wrong is handed.
 * Subsequently the connection is setted up and the GET-request ist send. After that,
 * the response of the server is getting parsed, checked and read (eventually using gziputil routines).
 * Finally the response is written to either the outputfile (or a file with that name at a specified path) or to stdout.
 * @param argc the argument count containing the number of options and arguments
 * @param argv the argument vector contatining the program-name [0], options and arguments.
 * @return 0 on success, 1 in case of an arbritrary error, 2 if the server sends a malformed
 *response and 3 if the HTTP-response code is  not 200. 
 * */
int main(int argc, char *argv[])
{
    PROGRAM_NAME = argv[0];

    char c;
    int p_count = 0, o_count = 0, d_count = 0;
    char *URL = NULL, *port = NULL, *d_path = NULL;
    FILE *out = stdout;
    while ((c = getopt(argc, argv, "p:o:d:")) != -1)
    {
        switch (c)
        {
        case 'p':
            p_count++;
            if (p_count > 1)
            {
                cleanup(NULL, NULL, NULL, NULL, out);
                usage();
            }
            port = optarg;
            break;
        case 'o':
            o_count++;
            if (o_count > 1)
            {
                if (fclose(out) < 0) // already an output file might be opened
                    error("fclose failed!", strerror(errno), PROGRAM_NAME);
                usage();
            }
            else
            {
                if ((out = fopen(optarg, "w")) == NULL)
                    error("fopen failed!", strerror(errno), PROGRAM_NAME);
            }
            break;
        case 'd':
            d_count++;
            if (d_count > 1)
                usage();
            d_path = strdup(optarg);
            break;
        default:
            usage();
            break;
        }
    }

    if (d_count > 0 && o_count > 0)
    {
        cleanup(NULL, NULL, d_path, NULL, out);
        usage();
    }

    if (argv[optind] == NULL) // no URL given
    {
        cleanup(NULL, NULL, d_count == 0 ? NULL : d_path, NULL, out);
        usage();
    }
    URL = argv[optind];

    if (port == NULL)
        port = DEFAULT_PORT;
    if (!is_valid_port(port))
    {
        cleanup(NULL, NULL, d_count == 0 ? NULL : d_path, NULL, out);
        error("Non valid port given!", "", PROGRAM_NAME);
    }
    if (!is_valid_url(URL))
    {
        cleanup(NULL, NULL, d_count == 0 ? NULL : d_path, NULL, out);
        error("Non valid URL given!", "", PROGRAM_NAME);
    }

    char *host = calloc(sizeof(char) * strlen(URL), sizeof(char));
    char *path = calloc(sizeof(char) * strlen(URL), sizeof(char));
    if (host == NULL || path == NULL)
    {
        cleanup(NULL, NULL, d_count == 0 ? NULL : d_path, NULL, out);
        error("malloc failed", strerror(errno), PROGRAM_NAME);
    }

    if (extract_host(URL, path, host) < 0)
    {
        cleanup(path, host, d_path, NULL, NULL);
        error("Failed to extract host!", strerror(errno), PROGRAM_NAME);
    }
    if (d_count == 1) // d-option specified
    {
        if (get_file_name(&d_path, path) < 0)
        {
            cleanup(path, host, d_path, NULL, NULL);
            error("Failed to extract file name!", strerror(errno), PROGRAM_NAME);
        }
        if ((out = fopen(d_path, "w")) == NULL) // out updated to specifc file
        {
            {
                cleanup(path, host, d_path, NULL, NULL);
                error("failed top open output file!", strerror(errno), PROGRAM_NAME);
            }
        }
    }

    int sockfd, res;
    if ((res = setup_socket(host, port, &sockfd)) != 0)
    {
        cleanup(path, host, d_path, NULL, out);
        error("Faield to setupsocket!", res == -1 ? strerror(errno) : gai_strerror(res), PROGRAM_NAME);
    }

    FILE *sockfile = fdopen(sockfd, "r+"); //r+ becasuse reading and writing needed
    if (sockfile == NULL)
    {
        cleanup(path, host, d_path, NULL, out);
        error("fdopen failed when opening sockfile!", strerror(errno), PROGRAM_NAME);
    }

    // this is needed, because we don't have to implement Transfer-Encoding, and therefore it should only
    // encode files, if we connect to the lab-host!
    if (strcmp(host, "pan.vmars.tuwien.ac.at") != 0)
    {
        if (fprintf(sockfile, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host) < 0)
        {
            cleanup(path, host, d_path, sockfile, out);
            error("fprintf failed while writing to socket!", strerror(errno), PROGRAM_NAME);
        }
    }
    else
    {
        if (fprintf(sockfile, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept-Encoding: gzip\r\n\r\n", path, host) < 0)
        {
            cleanup(path, host, d_path, sockfile, out);
            error("fprintf failed while writing to socket!", strerror(errno), PROGRAM_NAME);
        }
    }

    fflush(sockfile);
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    // check header
    if ((nread = getline(&line, &len, sockfile)) != -1)
    {
        char temp[1024]; //needed to print response code description
        memset(temp, 0, 1024);
        int res;
        if (!is_header_valid(line, temp, &res)) // invalid header was send
        {
            free(line);
            cleanup(path, host, d_path, sockfile, out);
            fprintf(stderr, "Protocol error!");
            exit(2);
        }
        if (res != 200) // not "OK" status code recveived
        {
            free(line);
            cleanup(path, host, d_path, sockfile, out);
            fprintf(stderr, "%s", temp);
            exit(3);
        }
    }

    bool encoded = false;
    
    // skipping header
    while ((nread = getline(&line, &len, sockfile)) != -1 && (strcmp(line, "\r\n") != 0))
    {
        if (strncmp(line, "Content-Encoding", 16) == 0) // server sent he can encode
        {
            if (strstr(line, "gzip") != NULL) // server can encode in gzip
                encoded = true;
        }
    }
    // this is needed, because we don't have to implement Transfer-Encoding, and therefore it should only
    // encode files, if we connect to the lab-host!
    if (strcmp(host, "pan.vmars.tuwien.ac.at") != 0)
    {
        encoded = false;
    }

    free(line);

    if (read_response_write(out, sockfile, encoded) < 0)
    {
        cleanup(path, host, d_path, sockfile, out);
        error("An error occured while writing/reading from socket", strerror(errno), PROGRAM_NAME);
    }
    if (ferror(sockfile) != 0)
    {
        cleanup(path, host, d_path, sockfile, out);
        error("An error occured while writing/reading on socket (ferror)", strerror(errno), PROGRAM_NAME);
    }
    if (ferror(out) != 0)
    {
        cleanup(path, host, d_path, sockfile, out);
        error("An error occured while writing/reading on outputfile (ferror)", strerror(errno), PROGRAM_NAME);
    }

    cleanup(path, host, d_path, sockfile, out);
    return EXIT_SUCCESS;
}

static int read_response_write(FILE *out, FILE *sockfile, bool encoded)
{
    if (encoded)
    {
        if (decompress_gzip(out, sockfile) < 0)
        {
            return -1;
        }
    }
    else
    {
        char buffer[128];
        memset(buffer, 0, 128);
        int res_;
        while ((res_ = fread(buffer, 1, 128, sockfile)) == 128)
        {
            buffer[res_] = '\0';
            if (fwrite(buffer, 1, res_, out) <= 0 || ferror(out) || ferror(sockfile))
            {
                return -1;
            }
            memset(buffer, 0, 128);
        }
        if (feof(sockfile))
        {
            buffer[res_] = '\0';
            if (fwrite(buffer, 1, res_, out) != res_ || ferror(out) || ferror(sockfile))
            {
                return -1;
            }
        }
    }
    fflush(out);
    return 0;
}

static int setup_socket(char *host, char *port, int *sockfd)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int res;
    if ((res = getaddrinfo(host, port, &hints, &ai)) != 0)
    {
        return res;
    }

    *sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (*sockfd < 0)
    {
        freeaddrinfo(ai);
        return -1;
    }
    if (connect(*sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        freeaddrinfo(ai);
        return -1;
    }
    freeaddrinfo(ai);
    return 0;
}

static void cleanup(char *path, char *host, char *dpath, FILE *sockfile, FILE *out)
{
    free(path);
    free(host);
    free(dpath);
    if (sockfile != NULL)
    {
        if (fclose(sockfile) < 0)
        {
            error("fclose failed!", strerror(errno), PROGRAM_NAME);
        }
    }

    if (out != NULL)
    {
        if (fclose(out) < 0)
        {
            error("fclose failed!", strerror(errno), PROGRAM_NAME);
        }
    }
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s [-p PORT] [-o FILE | -d DIR] URL \n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}
