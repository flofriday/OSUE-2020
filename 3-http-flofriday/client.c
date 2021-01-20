/**
 * @file client.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 19.12.2020
 * 
 * @brief The client program.
 * 
 * Implementation of a simple HTTP client with just the use of the C standard 
 * library.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <zlib.h>

/**
 * Internal buffer size.
 * @brief This size is used to create buffers when reading from a file.
 **/
#define BUFFER_SIZE 1024

/**
 * The name of the current program.
 */
const char *prog_name;

/**
 * Print usage.
 * @brief Print the usage of the client.
 * @details Reads the global variable prog_name.
 */
static void usage(void)
{
    fprintf(stderr, "[%s] USAGE: %s [-p PORT] [-o FILE | -d DIR] URL\n",
            prog_name, prog_name);
}

/**
 * @brief Copy a file.
 * @details Copies in chunks of the size BUFFER_SIZE.
 * @param dst The destination file.
 * @param src The source file.
 */
static void copy_file(FILE *dst, FILE *src)
{
    uint8_t buf[BUFFER_SIZE];
    while (!feof(src))
    {
        size_t read = fread(buf, sizeof(uint8_t), BUFFER_SIZE, src);
        fwrite(buf, sizeof(uint8_t), read, dst);
    }
}

/**
 * @brief Compy a file and gzip decompress it on the way.
 * @details Copies in chunks of the size BUFFER_SIZE.
 * @param dst The destination file.
 * @param src The source file.
 */
static void copy_compressed_file(FILE *dst, FILE *src)
{
    // Open the source file with zlib
    int fd = dup(fileno(src));
    gzFile gzsrc = gzdopen(fd, "r");
    if (gzsrc == NULL)
    {
        fprintf(stderr, "gzdopen failed\n");
        return;
    }

    uint8_t buf[BUFFER_SIZE];
    while (!gzeof(gzsrc))
    {
        size_t read = gzread(gzsrc, buf, BUFFER_SIZE);
        if (read == -1)
        {
            fprintf(stderr, "gzfread failed\n");
            break;
        }
        fwrite(buf, sizeof(uint8_t), read, dst);
    }
    gzclose(gzsrc);
}

/**
 * @brief Copy a compressed and chunk-encoded file.
 * @details May use the global variable prog_name and will write to stderr on 
 * failure.
 * @param dst The destination file.
 * @param src The source file.
 */
static void copy_chunked_compressed_file(FILE *dst, FILE *src)
{
    while (true)
    {
        // Read the chunk size
        char hex_size[10];
        int hex_size_len = 0;
        for (hex_size_len = 0; hex_size_len < 10; hex_size_len++)
        {
            fread(hex_size + hex_size_len, sizeof(char), 1, src);
            if (hex_size[hex_size_len] == '\n')
            {
                break;
            }
        }
        char *endptr;
        long size = strtol(hex_size, &endptr, 16);
        if (endptr != hex_size + (hex_size_len - 1))
        {
            fprintf(stderr, "strtol failed\n");
            return;
        }
        if (size == 0)
        {
            // This is the idicator that the response ended
            break;
        }

        uint8_t chunk[size];
        fread(chunk, sizeof(uint8_t), size, src);

        // Decompress the chunk
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        int err = inflateInit2(&stream, 16 + MAX_WBITS);
        if (err != Z_OK)
        {
            fprintf(stderr, "[%s] ERROR: Decompression Init returned: %d\n",
                    prog_name, err);
            return;
        }
        stream.next_in = chunk;
        stream.avail_in = size;
        uint8_t output[size];

        // Read some bytes and write them to the output file
        while (stream.avail_in > 0)
        {
            stream.next_out = output;
            stream.avail_out = size;
            err = inflate(&stream, true);
            if (err < 0)
            {
                fprintf(stderr, "[%s] ERROR: Decompression returned: %d\n",
                        prog_name, err);
                return;
            }
            fwrite(output, sizeof(uint8_t), size - stream.avail_out, dst);
        }
        inflateEnd(&stream);

        // Read the chunk end
        uint8_t chunkend[2];
        fread(chunkend, sizeof(uint8_t), 2, src);
        if (chunkend[0] != '\r' || chunkend[1] != '\n')
        {
            fprintf(stderr, "[%s] WARNING: Chunk ended wrong: %x %x\n",
                    prog_name, chunkend[0], chunkend[1]);
            return;
        }
    }
}

/**
 * @brief Parse an URL.
 * @details The caller must make sure that the host and resource arrays are 
 * large enough to hold host and resource.
 * @param host The destination to save the parsed host.
 * @param resource The destination to save the parsed resource
 * @param url A string of the complete url.
 * @return Upon success 0 is returned, otherwise -1;
 */
static int parse_url(char *host, char *resource, char *url)
{
    // Check that the url starts with "http://"
    if (strncmp(url, "http://", 7) != 0)
    {
        fprintf(stderr, "[%s] ERROR: Invalid url (scheme missing).\n",
                prog_name);
        return -1;
    }

    // Calculate where the host begins
    char *hostptr = &url[7];
    char *hostendptr = &url[strlen(url)];

    // Calculate where the host ends
    char *resptr = strchr(hostptr, '/');
    char *argptr = strchr(hostptr, '?');
    if (resptr != NULL)
    {
        hostendptr = resptr;
    }
    else if (argptr != NULL)
    {
        hostendptr = argptr;
    }
    strncpy(host, hostptr, (hostendptr - hostptr));
    host[hostendptr - hostptr] = '\0';

    // Calculate the resource
    if (resptr != NULL)
    {
        strcpy(resource, resptr);
        return 0;
    }

    strcpy(resource, "/");

    if (argptr != NULL)
    {
        strcat(resource, argptr);
    }

    return 0;
}

/**
 * @brief Open the right file to write the response payload to.
 * @param file The output filename, can be NULL.
 * @param dir The output dirname, can be NULL
 * @param res The resource of the URL.
 * @return Upon success a iostream file to write to, otherwise NULL.
 */
static FILE *open_output(char *file, char *dir, char *res)
{
    // No file or directory provided
    if (file == NULL && dir == NULL)
    {
        return stdout;
    }

    FILE *f;
    if (file != NULL)
    {
        f = fopen(file, "w");
    }
    else
    {
        size_t dirlen = strlen(dir);
        size_t reslen = strlen(res);
        char tmp[dirlen + reslen + 12];
        strcpy(tmp, dir);
        if (dir[dirlen - 1] != '/')
        {
            strcat(tmp, "/");
        }
        if (res[reslen - 1] == '/')
        {
            strcat(tmp, "index.html");
        }
        else
        {
            char *res_file = strrchr(res, '/');
            if (res_file == NULL)
            {
                res_file = res;
            }
            strcat(tmp, res_file);
        }
        f = fopen(tmp, "w");
    }

    return f;
}

/**
 * @brief Create a connection to the server, so that this client can send
 * the server a request to the server.
 * @details This function does just create the connection but does not send 
 * any request to the server.
 * This function will write to stderr error messages if an error occours.
 * @param host The host as a string to connect to.
 * @param port The port as a string to connect to.
 * @return Upon success a file to send the request to, otherwiese NULL.
 */
static FILE *create_connection(char *host, char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ai_err;
    if ((ai_err = getaddrinfo(host, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "[%s] ERROR: Unable to get addr info: %s\n",
                prog_name, gai_strerror(ai_err));
        return NULL;
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd == -1)
    {
        fprintf(stderr, "[%s] ERROR: Unable to create a socket: %s\n",
                prog_name, strerror(errno));
        freeaddrinfo(ai);
        return NULL;
    }

    if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) == -1)
    {
        fprintf(stderr, "[%s] ERROR: Unable to connect: %s\n",
                prog_name, strerror(errno));
        freeaddrinfo(ai);
        return NULL;
    }

    FILE *sockfile = fdopen(sockfd, "r+");
    if (sockfile == NULL)
    {
        fprintf(stderr, "[%s] ERROR: Unable to fdopen: %s\n",
                prog_name, strerror(errno));
        freeaddrinfo(ai);
        return NULL;
    }
    setvbuf(sockfile, NULL, _IONBF, 0);

    freeaddrinfo(ai);
    return sockfile;
}

/**
 * @brief Send a HTTP/1.1 GET request to a server.
 * @details This function sends a complete request to the server and after this 
 * call conn_file should not be written to.
 * @param conn_file A file to wirte the request to.
 * @param host The hostname as a string.
 * @param resource The resource to request.
 */
static void send_request(FILE *conn_file, char *host, char *resource)
{
    fprintf(conn_file, "\
GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Accept-Encoding: gzip\r\n\
Connection: close\r\n\r\n",
            resource,
            host);
    fflush(conn_file);
}

/**
 * @brief Parse the response from the server and wirte the payload to a file. 
 * In the case the server send a gziped or chunk-encoded payload this function 
 * will decode it before writing it.
 * @details May write errors to stderr.
 * @param out_file The file to write the payload to.
 * @param conn_file A file to read the request from.
 * @return Upon success 0, in case of HTTP/1.1 violations 2, in case the server
 * responded with a non 200 status 3 and otherwise 1.
 */
static int read_response(FILE *out_file, FILE *conn_file)
{
    size_t cap = 0;
    char *line = NULL;

    // Read and parse the first line as it contains special information that
    // must be parsed
    if (getline(&line, &cap, conn_file) == -1)
    {
        // No line was sent by the server
        fprintf(stderr, "[%s] ERROR: Protocol error!\n",
                prog_name);
        free(line);
        return 2;
    }

    char *part = strtok(line, " ");
    if (part == NULL || strcmp(part, "HTTP/1.1") != 0)
    {
        // There is no space in the first line or it does not start with
        // HTTP/1.1
        fprintf(stderr, "[%s] ERROR: Protocol error!\n",
                prog_name);
        free(line);
        return 2;
    }

    char *status_code = strtok(NULL, " ");
    char *status_text = strtok(NULL, "\r\n");
    if (status_code == NULL || status_text == NULL)
    {
        fprintf(stderr, "[%s] ERROR: Protocol error!\n",
                prog_name);
        free(line);
        return 2;
    }

    char *endptr;
    strtol(status_code, &endptr, 10);
    if (endptr != status_code + strlen(status_code))
    {
        // The statuscode is not a number
        fprintf(stderr, "[%s] ERROR: Protocol error!\n",
                prog_name);

        free(line);
        return 2;
    }

    if (strncmp(status_code, "200", 3) != 0)
    {
        // Statuscode is not 200
        fprintf(stderr, "[%s] STATUS: %s %s\n",
                prog_name, status_code, status_text);
        free(line);
        return 3;
    }

    // Read the rest of the headers line by line
    bool is_compressed = false;
    bool is_chunked = false;
    while (true)
    {
        if (getline(&line, &cap, conn_file) == -1)
        {
            fprintf(stderr, "[%s] ERROR: No content.\n",
                    prog_name);
            free(line);
            return 1;
        };

        if (strcmp(line, "\r\n") == 0)
        {
            break;
        }

        if (strncmp(line, "Content-Encoding: gzip", strlen("Content-Encoding: gzip")) == 0)
        {
            is_compressed = true;
        }

        if (strncmp(line, "Transfer-Encoding", strlen("Transfer-Encoding")) == 0 &&
            strstr(line, "chunked") != NULL)
        {
            is_chunked = true;
        }
    }
    free(line);

    // Read the rest of the response as binary data
    if (is_compressed && is_chunked)
    {
        copy_chunked_compressed_file(out_file, conn_file);
    }
    else if (is_compressed)
    {
        copy_compressed_file(out_file, conn_file);
    }
    else
    {
        copy_file(out_file, conn_file);
    }

    return 0;
}

/**
 * @brief The entrypoint of the client. Execution of the client always starts
 * and ends here.
 * @details Uses the global variable prog_name.
 * @param argc The argument counter
 * @param argc The argument vector
 * @return Upon success EXIT_SUCCESS, on HTTP protocol violation 2, upon 
 * unsuccessful requests 3 and on all other errors EXIT_FAILURE.
 */
int main(int argc, char *argv[])
{
    prog_name = argv[0];
    bool has_p = false;
    char *port = "80";
    char *filename = NULL;
    char *dirname = NULL;
    int c;
    while ((c = getopt(argc, argv, "p:o:d:")) != -1)
    {
        switch (c)
        {
        case 'p':
            if (has_p)
            {
                fprintf(stderr, "[%s] ERROR: -p can only appear once.\n",
                        prog_name);
                usage();
                exit(EXIT_FAILURE);
            }
            has_p = true;
            port = optarg;
            break;
        case 'o':
            if (filename != NULL)
            {
                fprintf(stderr, "[%s] ERROR: -o can only appear once.\n",
                        prog_name);
                usage();
                exit(EXIT_FAILURE);
            }
            filename = optarg;
            break;
        case 'd':
            if (dirname != NULL)
            {
                fprintf(stderr, "[%s] ERROR: -d can only appear once.\n",
                        prog_name);
                usage();
                exit(EXIT_FAILURE);
            }
            dirname = optarg;
            break;
        case '?':
            exit(EXIT_FAILURE);
            break;
        default:
            assert(false);
        }

        if (filename != NULL && dirname != NULL)
        {
            fprintf(stderr, "[%s] ERROR: -d and -o cannot appear together.\n",
                    prog_name);
            usage();
            exit(EXIT_FAILURE);
        }
    }

    // Read the url
    if (optind != argc - 1)
    {
        usage();
        exit(EXIT_FAILURE);
    }
    char *url = argv[optind];
    size_t url_len = strlen(url);

    // Parse the url
    char url_host[url_len];
    char url_resource[url_len];
    if (parse_url(url_host, url_resource, url) == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Create the output file
    FILE *out_file = open_output(filename, dirname, url_resource);
    if (out_file == NULL)
    {
        fprintf(stderr, "[%s] ERROR: Unable to open output file: %s\n",
                prog_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Create the socket
    FILE *conn_file = create_connection(url_host, port);
    if (conn_file == NULL)
    {
        fclose(out_file);
        exit(EXIT_FAILURE);
    }

    // Send the request
    send_request(conn_file, url_host, url_resource);

    // Read the response
    int exit_code;
    if ((exit_code = read_response(out_file, conn_file)) != 0)
    {
        fclose(conn_file);
        fclose(out_file);
        exit(exit_code);
    }

    // Free all resources
    fclose(conn_file);
    fclose(out_file);

    exit(EXIT_SUCCESS);
}
