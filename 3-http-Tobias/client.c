/**
 * @file   client.c
 * @author Tobias de Vries (e01525369)
 * @date   29.12.2020
 *
 * @brief A program to read in a specified resource from a specified host via HTTP and write the response body to a specified target.
 *
 * @details The program receives an url as argument, opens a socket to connect to the so specified host
 * and reads the resource pointed to by the url, by sending a GET-Request.
 * If the response value does not start with the Protocol specified by HTTP_PROTOCOL or this is not followed by a status-code,
 * the program terminates with exit-code 2.
 * Otherwise, if the response status is not 200 (HTTP-OK) the program terminates with exit-code 3.
 * The program synopsis is as follows: client [-l] [-p PORT] [ -o FILE | -d DIR ] URL
 * The optional option -l indicates that additional log messages should be written to the target.
 * The optional option -p can be used to specify a port manually. If omitted the standard http-port 80 will be used.
 * The optional options -o or -d can be used to either specify a file the response body should be written to (will be created/overwritten),
 * or a directory in which a file matching the resources name will be created (if the resource is a directory the file is named index.html).
 * If both of those options are omitted the response body is printed to stdout.
 **/

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netdb.h>
#include <values.h>


/** Macro to ensure adding a terminating NULL argument (sentinel) at the end of the argument list is not forgotten. */
#define TRY_CONCAT(dest, ...) tryConcat(dest, __VA_ARGS__, NULL)

//region OPTIONS
#define URL_START              "http://" // String initiating every given url, followed by host
#define TARGET_FILE_OPTION     "w"
#define SERVER_STREAM_MODE     "r+"
#define HOST_TERMINATING_CHARS ";/?:@=&" // host is between URL_START and the first occurrence of any of these

#define DEFAULT_PORT      "80"         // If changed update program description
#define DEFAULT_TARGET    stdout       // If changed update program description
#define DEFAULT_FILE_NAME "index.html" // If changed update program description

#define SOCKET_DOMAIN          AF_INET
#define SOCKET_PROTOCOL        0
#define SOCKET_CONNECTION_TYPE SOCK_STREAM

#define HTTP_OK       200
#define HTTP_PROTOCOL "HTTP/1.1"

#define ERROR_PROTOCOL_STATUS   2 // If changed update program description
#define INVALID_RESPONSE_STATUS 3 // If changed update program description
//endregion

//region ERROR
/** Macro ensures correct line number for errors. */
#define TRY(result, message) try(result, message, __LINE__)

/** Macro ensures correct line number for errors. */
#define TRY_PTR(result, message) tryPtr(result, message, __LINE__)

//region MESSAGES
#define USAGE_ERROR_FORMAT "%s\nSYNOPSIS: %s [-p PORT] [-o FILE | -d DIR] URL\n"

#define ERROR_LOGGING                 "Writing to log failed"
#define ERROR_CONNECT                 "Could not connect to the server"
#define ERROR_OPEN_FILE               "Could not open/create the specified target file"
#define ERROR_INVALID_URL             "Invalid url, must start with 'http://'"
#define ERROR_GETTING_INFO            "Could not generate Address-Info"
#define ERROR_SEND_REQUEST            "Could not send request"
#define ERROR_SOCKET_CREATION         "Creating socket file descriptor failed"
#define ERROR_UNKNOWN_ARGUMENT        "Unknown argument"
#define ERROR_RESPONSE_PROTOCOL       "Protocol error!"
#define ERROR_OPEN_SERVER_STREAM      "Could not create stream to server"
#define ERROR_INVALID_ARGUMENT_NUMBER "Invalid number of arguments"
//endregion
//endregion

//region LOGGING
/** Macro to log message only if logging is enabled. */
#define LOG(format, ...)                                                  \
    if (logEnabled_g)                                                      \
        TRY(fprintf(settings_g.target, format, __VA_ARGS__), ERROR_LOGGING) \
//endregion

//region TYPES
/** Type to emulate a boolean value. Can be used for logical expressions. */
typedef enum {
    false = 0,
    true = 1
} bool;

/** A struct to hold the arguments passed to the program. */
typedef struct {
    char* url;
    char* port;
    char* host;
    char* programName;
    char* requestedResource;
    FILE* target;
} program_settings_t;
//endregion

//region GLOBAL VARIABLES
/** The arguments passed to the program in parsed form. */
program_settings_t settings_g = {0};

/** Specified over program options. Indicates whether or not additional info should be printed to the target. */
bool logEnabled_g = false;
//endregion

//region FUNCTION DECLARATIONS
static inline void parseUrl();
static inline void tryParseArguments(int argc, char **argv);
static inline void setupTargetInDirectory(const char *directory);

static inline void tryCreateGetRequest(char *path, char *protocol, char **request_out);
static inline void tryAddRequestHeader(char *header, char *value, bool last, char **request_out);
static inline char *getResponseBody(const char *response);

static inline void tryOpenConnection(FILE **serverStream_out);
static inline void trySendRequest(FILE *serverStream, char *request);
static inline void ensureValidResponse(char *response);

static inline void try(int operationResult, const char *message, int line);
static inline void tryPtr(void *operationResult, const char *message, int line);
static inline void printErrnoAndTerminate(const char *additionalMessage, int line);
static inline void terminateWithProtocolError();

static inline bool endsWith(const char *string, char character);
static inline void tryConcat(char **result_out, const char *str, ...);
static inline void tryPrintLine(FILE *target, const char *stringPtr);
static inline void tryReadStream(FILE *stream, char **result_out);
//endregion


/**
 * @brief The entry point of the program.
 * @details This function executes the whole program by calling upon other functions.
 *
 * global variables used: settings_g - The settings of the program specified directly and indirectly by the program arguments
 *
 * @param argumentCounter The argument counter.
 * @param argumentValues  The argument values.
 *
 * @return Returns EXIT_SUCCESS upon success, EXIT_FAILURE upon failure, ERROR_PROTOCOL_STATUS on protocol error and
 * INVALID_RESPONSE_STATUS if the response-status does not indicate success.
 */
int main(int argc, char *argv[])
{
    tryParseArguments(argc, argv);
    LOG("Port: %s\nFull url: %s\nHost: %s\nFilePath: %s",
        settings_g.port, settings_g.url, settings_g.host, settings_g.requestedResource);

    FILE *serverStream;
    tryOpenConnection(&serverStream);

    char *request; // Must be an allocated string with 0 termination
    TRY_PTR(request = calloc(1, 1), "calloc failed");

    tryCreateGetRequest(settings_g.requestedResource, HTTP_PROTOCOL, &request);
    tryAddRequestHeader("Host", settings_g.host, false, &request);
    tryAddRequestHeader("Connection", "close", true, &request);
    LOG("\n\nRequest:\n%s", request);

    trySendRequest(serverStream, request);

    char *response;
    TRY_PTR(response = malloc(1), "malloc failed");

    tryReadStream(serverStream, &response);
    LOG("Full Response:\n%s", response);

    ensureValidResponse(response);

    LOG("%s", "\nResponse Body:\n");
    fputs(getResponseBody(response), settings_g.target);

    free(request);
    free(response);
    free(settings_g.host);
    fclose(serverStream); // Also closes socket fd
    fclose(settings_g.target);
    return EXIT_SUCCESS;
}


//region ERROR HANDLING
/**
 * @brief Prints ERROR_RESPONSE_PROTOCOL to stdout and terminates the program with ERROR_PROTOCOL_STATUS.
 * @details Global variables used: settings_g - To close the target
 */
static inline void terminateWithProtocolError()
{
    fprintf(stdout, "%s", ERROR_RESPONSE_PROTOCOL);
    fclose(settings_g.target);
    exit(ERROR_PROTOCOL_STATUS);
}

/**
 * @brief Prints the specified message along with the program synopsis and terminates the program with EXIT_FAILURE.
 * @details Global variables used: settings_g
 *
 * @param message The message to print alongside the synopsis.
 */
static inline void printUsageErrorAndTerminate(const char *message)
{
    fprintf(stderr, USAGE_ERROR_FORMAT, message, settings_g.programName);
    exit(EXIT_FAILURE);
}

/**
 * @brief Prints a given message and line number as well as the program name, process id and current content of errno
 * to stderr (as well as the log file if LOGGING is true) and terminates the program with EXIT_FAILURE.
 *
 * global variables used: settings_g - To access the program name as specified by argv[0], close the target and free host.
 *
 * @param additionalMessage The custom message to print along with the program name and errno contents.
 * @param line              The line number to print along with the error message.
 */
static inline void printErrnoAndTerminate(const char *additionalMessage, int line)
{
    fprintf(stderr, "ERROR in %s (line %d) - %s; ERRNO: %s\n",
            settings_g.programName, line, additionalMessage, strerror(errno));

    if (settings_g.target != NULL)
        fclose(settings_g.target);

    if (settings_g.host != NULL)
        free(settings_g.host);

    exit(EXIT_FAILURE);
}

/**
 * @brief A function to check the result of an operation and terminate if it failed.
 * @details Use by passing the result of an operation that returns a negative value upon failure as first argument,
 * along with a message and the line number that should be printed in case the result is indeed negative.
 * If the given operation result is negative the program calls printErrnoAndTerminate
 * and thereby terminates the program with EXIT_FAILURE.
 *
 * @param operationResult The result of an operation that returns a negative value upon failure.
 * @param message         The error message that should be printed in case the result is indeed negative.
 * @param line            The line that should be printed in case the result is indeed negative.
 */
static inline void try(int operationResult, const char *message, int line)
{
    if (operationResult < 0)
        printErrnoAndTerminate(message, line);
}

/** Same as try but checks for a NULL pointer instead of a negative integer. */
static inline void tryPtr(void *operationResult, const char *message, int line)
{
    if (operationResult == NULL)
        printErrnoAndTerminate(message, line);
}
//endregion

//region ARGUMENT HANDLING
/**
 * @brief parses the given program arguments and stores them in settings_g.
 * @details Terminates with EXIT_FAILURE by calling printUsageErrorAndTerminate() if the given arguments are invalid or
 * by calling printErrnoAndTerminate if any called method fails.
 *
 * Global variables used: settings_g
 *
 * @param argc The number of arguments the program received.
 * @param argv The arguments the program received.
 */
static inline void tryParseArguments(int argc, char **argv)
{
    settings_g.programName = argv[0];

    int opt;
    bool portSpecified = false;
    bool fileSpecified = false;
    bool directorySpecified = false;
    char *directory;

    while ((opt = getopt(argc, argv, "lp:o:d:")) != -1)
    {
        switch (opt)
        {
            case 'l':
                if (logEnabled_g)
                    printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

                logEnabled_g = true;
                break;

            case 'p':
                if (portSpecified)
                    printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

                portSpecified = true;
                settings_g.port = optarg;
                break;

            case 'o':
                if (fileSpecified || directorySpecified)
                    printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

                fileSpecified = true;
                TRY_PTR(settings_g.target = fopen(optarg, TARGET_FILE_OPTION), ERROR_OPEN_FILE);
                break;

            case 'd':
                if (fileSpecified || directorySpecified)
                    printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

                directorySpecified = true;
                directory = optarg;
                break;

            default:
                printUsageErrorAndTerminate(ERROR_UNKNOWN_ARGUMENT);
        }
    }

    if (optind != argc - 1)
        printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

    if (portSpecified == false)
        settings_g.port = DEFAULT_PORT;

    settings_g.url = argv[optind];
    if (strncmp(settings_g.url, URL_START, strlen(URL_START)) != 0)
        printUsageErrorAndTerminate(ERROR_INVALID_URL);

    parseUrl();

    if (directorySpecified)
        setupTargetInDirectory(directory); // Must be called after parseUrl()
    else if (!fileSpecified)
        settings_g.target = DEFAULT_TARGET;
}

/**
 * @brief Extracts the host and the path to the requested Resource from settings_g.url and stores them in settings_g.
 * @details Global variables used: settings_g
 */
static inline void parseUrl()
{
    char *urlWithoutHttp = &settings_g.url[strlen(URL_START)];

    settings_g.requestedResource = strchr(urlWithoutHttp, '/');

    ulong hostLength = strlen(urlWithoutHttp);
    if (settings_g.requestedResource != NULL)
        hostLength -= strlen(strpbrk(urlWithoutHttp, HOST_TERMINATING_CHARS));

    TRY_PTR(settings_g.host = malloc(hostLength + 1), "malloc failed");

    strncpy(settings_g.host, urlWithoutHttp, hostLength);
    settings_g.host[hostLength] = '\0';
}

/**
 * @brief Used if the directory option is set to initialize the target.
 * @details A file matching the resources name will be created in the specified directory unless the resource is a directory,
 * in which case the file is named as specified in DEFAULT_FILE_NAME.
 * Requires parseUrl to be called first!
 * Terminates the program with EXIT_FAILURE by calling printErrnoAndTerminate if this is not the case
 * or if any called method fails.
 *
 * Global variables used: settings_g
 *
 * @param directory the name of the directory.
 */
static inline void setupTargetInDirectory(const char *directory)
{
    if (settings_g.requestedResource == NULL)
        printErrnoAndTerminate("Must call parseUrl() before setupTargetInDirectory().", __LINE__);

    char *file =
            settings_g.requestedResource == NULL /*requested empty path*/ || endsWith(settings_g.requestedResource, '/') ?
                DEFAULT_FILE_NAME :
                strrchr(settings_g.requestedResource, '/') + 1; // +1 to get substring starting after the last /

    char *target;
    TRY_PTR(target = calloc(1, 1), "calloc failed");

    TRY_CONCAT(&target, directory, "/", file);
    TRY_PTR(settings_g.target = fopen(target, TARGET_FILE_OPTION), ERROR_OPEN_FILE);
    free(target);
}
//endregion

//region HTTP CLIENT
/**
 * @brief Tries to open a connection to the server specified by setting_g and store it in serverStream_out.
 * Terminates the program with EXIT_FAILURE upon failure of any method.
 * @details Global variables used: settings_g
 *
 * @param serverStream_out A pointer to where the server-stream should be saved.
 */
static inline void tryOpenConnection(FILE **serverStream_out)
{
    struct addrinfo *addressInfo;
    struct addrinfo options = {
            .ai_family   = SOCKET_DOMAIN,
            .ai_socktype = SOCKET_CONNECTION_TYPE,
            .ai_protocol = SOCKET_PROTOCOL
    };

    int result = getaddrinfo(settings_g.host, settings_g.port, &options, &addressInfo);
    if (result != 0) {
        fprintf(stderr, "%s: %s\n", ERROR_GETTING_INFO, gai_strerror(result));
        freeaddrinfo(addressInfo);
        exit(EXIT_FAILURE);
    }

    int socketFd;
    TRY(socketFd = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol),
        ERROR_SOCKET_CREATION);

    TRY(connect(socketFd, addressInfo->ai_addr, addressInfo->ai_addrlen), ERROR_CONNECT);
    TRY_PTR(*serverStream_out = fdopen(socketFd, SERVER_STREAM_MODE), ERROR_OPEN_SERVER_STREAM);
    freeaddrinfo(addressInfo);
}

/**
 * @brief Tries to write the specified request to the specified serverStream. Terminates the program with EXIT_FAILURE
 * upon failure.
 *
 * @param serverStream A stream to the server the request should be written to.
 * @param request      The request to write.
 */
static inline void trySendRequest(FILE *serverStream, char *request)
{
    TRY(fputs(request, serverStream), ERROR_SEND_REQUEST);
    TRY(fflush(serverStream), ERROR_SEND_REQUEST);
}

/**
 * @brief Checks if the given response starts with HTTP_PROTOCOL followed by a status code.
 * If not the program is terminated with exit-code ERROR_RESPONSE_PROTOCOL.
 * Also verifies that the status code is HTTP_OK and otherwise terminates the program with INVALID_RESPONSE_STATUS.
 *
 * @param response The response to verify.
 */
static inline void ensureValidResponse(char *response)
{
    if (strncmp(response, HTTP_PROTOCOL, strlen(HTTP_PROTOCOL)) != 0)
        terminateWithProtocolError();

    char *nextChar;
    char *statusStart = response + strlen(HTTP_PROTOCOL) + 1;
    long status = strtol(statusStart, &nextChar, 10);

    if (nextChar == statusStart || *nextChar != ' ' || status == LONG_MIN || status == LONG_MAX)
        terminateWithProtocolError();

    if (status != HTTP_OK)
    {
        tryPrintLine(stdout, statusStart);
        fclose(settings_g.target);
        exit(INVALID_RESPONSE_STATUS);
    }
}
//endregion

//region UTILITY
//region HTTP
/**
 * @brief Creates a GET-Request in the form "GET <path> <protocol>\r\n".
 * @details Terminates the program with EXIT_FAILURE if concatenating fails.
 *
 * @param path        The path of the resource to get. If null '/' is assumed as path.
 * @param protocol    The protocol used for communication.
 * @param request_out A pointer to where the result should be stored. Must be an allocated address-space.
 */
static inline void tryCreateGetRequest(char *path, char *protocol, char **request_out) {
    TRY_CONCAT(request_out, "GET ", path == NULL ? "/" : path, " ", protocol, "\r\n");
}

/**
 * @brief Adds a Header in the form "<header>: <value>\r\n" to a given GET-Request.
 * If last is true an additional '\r\n' is added to indicate completion of the header-section.
 * @details Reallocates request_out. Terminates the program with EXIT_FAILURE if concatenating fails.
 *
 * @param header      The name of the header.
 * @param value       The value of the header.
 * @param last        Indicates if this header finishes the header-section.
 * @param request_out A pointer to where the result should be stored. Must be an allocated address-space.
 */
static inline void tryAddRequestHeader(char *header, char *value, bool last, char **request_out) {
    TRY_CONCAT(request_out, header, ": ", value, "\r\n", last ? "\r\n" : "");
}

/**
 * @brief Returns a pointer to the substring of response following the first empty line (and thereby the header section).
 * @param response Any http-response including a header.
 * @return A pointer to the beginning of the response body.
 */
static inline char *getResponseBody(const char *response) {
    return strstr(response, "\r\n\r\n") + 4;
}
//endregion

//region STREAMS
/**
 * @brief Reads from the specified stream until EOF is read and stores the result in result_out.
 * @details result_out must be a pointer to an allocated address-space. Will be reallocated.
 * If reallocating result_out fails, the program calls printErrnoAndTerminate
 * and thereby terminates the program with EXIT_FAILURE.
 *
 * @param stream     The stream to read from.
 * @param result_out A pointer to where the result should be stored. Must be an allocated address-space.
 */
static inline void tryReadStream(FILE *stream, char **result_out)
{
    int character;

    int i = 0;
    for (; (character = fgetc(stream)) != EOF; i++)
    {
        TRY_PTR(*result_out = realloc(*result_out, i+2), "realloc failed.");
        (*result_out)[i] = (char) character;
    }
    (*result_out)[i] = '\0';
}

/**
 * @brief Prints the string pointed to by stringPtr to the specified target using fputc
 * until one of the following characters is encountered: '\r', '\n' or '\0'.
 * @details Terminates with EXIT_FAILURE by calling printErrnoAndTerminate if fputc fails.
 *
 * @param target    The stream to print the line to.
 * @param stringPtr The pointer to the beginning of the line.
 */
static inline void tryPrintLine(FILE *target, const char *stringPtr)
{
    for (int i = 0; *(stringPtr + i) != '\r' && *(stringPtr + i) != '\n' && *(stringPtr + i) != '\0'; i++) {
        TRY(fputc(*(stringPtr + i), target), "fputc failed");
    }
    fflush(stdout);
}
//endregion

//region STRINGS
/**
 * @brief Determines weather a given string ends with a given character.
 *
 * @param string    The string to check.
 * @param character The character in question.
 * @return true (1) if the last character of the given string is the given character, false (0) otherwise.
 */
static inline bool endsWith(const char *string, char character) {
    return string[strlen(string)-1] == character;
}

/**
 * @brief Concatenates any number of given strings and stores the result in result_out.
 * @details result_out must be a pointer to an allocated address-space containing a \0 terminated string.
 * The last argument must be NULL.
 * If reallocating result_out fails, the program calls printErrnoAndTerminate
 * and thereby terminates the program with EXIT_FAILURE.
 *
 * @param result_out A pointer to an allocated address-space that the result should be stored in.
 * @param str The first string to concatenate.
 * @param ... The remaining strings to concatenate. Last argument must be NULL.
 */
static inline void tryConcat(char **result_out, const char *str, ...)
{
    va_list arg;
    ulong size = strlen(*result_out)+1;

    va_start(arg, str);
    while (str)
    {
        size += strlen(str);
        TRY_PTR(*result_out = realloc(*result_out, size), "realloc failed.");

        strcat(*result_out, str);
        str = va_arg(arg, const char *);
    }
    va_end(arg);
}
//endregion
//endregion