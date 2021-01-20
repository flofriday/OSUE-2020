/**
 * @file   server.c
 * @author Tobias de Vries (e01525369)
 * @date   29.12.2020
 *
 * @brief A program to provide resources to clients via HTTP.
 *
 * @details The program receives a root path as argument, opens a socket to wait for connection-requests
 * and provides the content of the requested file if possible. Only GET-Requests are supported.
 * If the request header does not consist of request-method, path and the protocol specified by HTTP_PROTOCOL,
 * the server responds with 400 Bad Request. If the request-method is not GET, 501 Not Implemented is returned.
 * Otherwise, a header indicating success (200 OK) and the content of the requested file are returned
 * if the file can be opened by the server and 404 Not found if the server can't open it.
 * The program synopsis is as follows: server [-l] [-p PORT] [-i INDEX] DOC_ROOT
 * The optional option -l indicates that additional log messages should be written to stdout.
 * The optional option -p can be used to specify a port manually. If omitted DEFAULT_PORT will be used.
 * The optional option -i can be used to specify the default filename in case the client requests a directory path.
 * If omitted the default filename for this case is as specified in DEFAULT_INDEX.
 * The argument DOC_ROOT indicates the root all requested file-paths are relative to.
 *
 * For example, if the server receives a request with the first line 'GET /file/ HTTP/1.1' and was started without any options,
 * it will look for DOC_ROOT/file/DEFAULT_INDEX.
 **/

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>


/** Macro to ensure adding a terminating NULL argument (sentinel) at the end of the argument list is not forgotten. */
#define TRY_CONCAT(dest, ...) tryConcat(dest, __VA_ARGS__, NULL)

#define MAX_LONG_STRING   10 // LONG_MAX is 2147483647
#define MAX_RFC822_STRING 28 // aaa, dd bbb yy hh:mm:ss GMT\0

//region OPTIONS
#define CONNECTION_MODE    "r+"
#define TARGET_FILE_OPTION "r"

#define DEFAULT_PORT  "8080"
#define DEFAULT_INDEX "index.html"

#define BACKLOG                1
#define SOCKET_DOMAIN          AF_INET
#define ADDR_INFO_FLAGS        AI_PASSIVE
#define SOCKET_PROTOCOL        0
#define SOCKET_CONNECTION_TYPE SOCK_STREAM

#define SOCKET_OPTION_LVL   SOL_SOCKET
#define SOCKET_OPTION_NAME  SO_REUSEADDR
#define SOCKET_OPTION_VALUE 1

#define HTTP_GET      "GET"
#define HTTP_PROTOCOL "HTTP/1.1"
//endregion

//region ERROR
/** Macro ensures correct line number for errors. */
#define TRY(result, message) try(result, message, __LINE__)

/** Macro ensures correct line number for errors. */
#define TRY_PTR(result, message) tryPtr(result, message, __LINE__)

//region MESSAGES
#define USAGE_ERROR_FORMAT "%s\nSYNOPSIS: %s [-p PORT] [-i INDEX] DOC_ROOT\n"

#define ERROR_BIND                    "Could not bind to the socket"
#define ERROR_CONNECT                 "Could not connect to the client"
#define ERROR_LOGGING                 "Writing to log failed"
#define ERROR_LISTENING               "Listening for connections failed"
#define ERROR_GETTING_INFO            "Could not generate Address-Info"
#define ERROR_SEND_RESPONSE           "Could not send response"
#define ERROR_SOCKET_CREATION         "Creating socket file descriptor failed"
#define ERROR_UNKNOWN_ARGUMENT        "Unknown argument"
#define ERROR_DETERMINING_FILESIZE    "Could not determine the size of requested file"
#define ERROR_INVALID_ARGUMENT_NUMBER "Invalid number of arguments"
//endregion
//endregion

//region LOGGING
/** Macro to log message only if logging is enabled. Uses the global variable logEnabled_g. */
#define LOG(format, ...)                                       \
    if (logEnabled_g)                                           \
        TRY(fprintf(stdout, format, __VA_ARGS__), ERROR_LOGGING) \
//endregion

//region TYPES
/** Type to emulate a boolean value. Can be used for logical expressions. */
typedef enum {
    false = 0,
    true = 1
} bool;

/** A struct to hold the arguments passed to the program. */
typedef struct {
    char* port;
    char* root;
    char* index;
    char* programName;
} program_settings_t;
//endregion

//region GLOBAL VARIABLES
/** The arguments passed to the program in parsed form. */
program_settings_t settings_g = {0};

/** Specified over program options. Indicates whether or not additional info should be printed to the target. */
bool logEnabled_g = false;

/** Yet via interrupt to tell the application to shut down. */
volatile sig_atomic_t shutdownInitiated_g = false;
//endregion

//region FUNCTION DECLARATIONS
static inline void tryParseArguments(int argc, char **argv);
static inline void initializeSignalHandler(int signal, void (*handler)(int), struct sigaction *sa_out);

static inline void tryStartListening(int *socketFd_out);
static inline void trySend(FILE *connection, char *data);
static inline void trySendEmptyResponse(FILE *connection, char *statusCode, char *statusDesc);
static inline char *tryProcessRequest(char *request, FILE *connection);
static inline void trySendFile(FILE *connection, char *requestedFilePath);

static inline void tryCreateResponseHeader(char *protocol, char *statusCode, char *statusDesc, char **response_out);
static inline void tryAddResponseHeader(char *header, char *value, bool last, char **response_out);

static inline void try(int operationResult, const char *message, int line);
static inline void tryPtr(void *operationResult, const char *message, int line);
static inline void printErrnoAndTerminate(const char *additionalMessage, int line);

static inline bool endsWith(const char *string, char character);
static inline void tryConcat(char **result_out, const char *str, ...);
static inline void tryGetSize(FILE *file, char (*size_out)[MAX_LONG_STRING]);
static inline void tryReadFile(FILE *file, char **content_out);
static inline void tryReadRequest(FILE *connection, char **request_out);
static inline void getDateInRFC822(char (*date_out)[MAX_RFC822_STRING]);
//endregion


/**
 * @brief A Signal-handler for SIGINT & SIGTERM Signals. Initiates shutdown of application.
 * @details Global variables used: shouldTerminate_g.
 *
 * @param _ required by sigaction.sa_handler. Not used.
 */
void initiateTermination(int _) {
    shutdownInitiated_g = true;
}

/**
 * @brief The entry point of the program.
 * @details This function executes the whole program by calling upon other functions.
 *
 * global variables used: settings_g - The settings of the program specified directly and indirectly by the program arguments
 *                        shutdownInitiated_g - to determine when to stop the program
 *
 * @param argumentCounter The argument counter.
 * @param argumentValues  The argument values.
 *
 * @return Returns EXIT_SUCCESS upon success, EXIT_FAILURE upon failure, ERROR_PROTOCOL_STATUS on protocol error and
 * INVALID_RESPONSE_STATUS if the response-status does not indicate success.
 */
int main(int argc, char *argv[])
{
    struct sigaction sigInt;
    struct sigaction sigTerm;
    initializeSignalHandler(SIGINT, initiateTermination, &sigInt);
    initializeSignalHandler(SIGTERM, initiateTermination, &sigTerm);

    tryParseArguments(argc, argv);

    int socketFd;
    tryStartListening(&socketFd);

    while(!shutdownInitiated_g)
    {
        int connectionFd;
        TRY(connectionFd = accept(socketFd, NULL, NULL), ERROR_CONNECT);

        FILE *connection;
        TRY_PTR(connection = fdopen(connectionFd, CONNECTION_MODE), ERROR_CONNECT);

        char *request;
        TRY_PTR(request = calloc(1, 1), "calloc failed");
        tryReadRequest(connection, &request);

        LOG("Request:\n%s", request);

        char *requestedFilePath = tryProcessRequest(request, connection);
        if (requestedFilePath != NULL)
            trySendFile(connection, requestedFilePath);

        free(request);
        fclose(connection);
    }

    return EXIT_SUCCESS;
}


/**
 * @brief Initializes a sigaction to act as specified by handler when the program receives the specified signal.
 *
 * @param signal  The signal that should be handled in a certain way.
 * @param handler The handler that should be called upon receiving the specified signal.
 * @param sa_out  The pointer to the sigaction that should be initialized.
 */
static inline void initializeSignalHandler(int signal, void (*handler)(int), struct sigaction *sa_out)
{
    memset(sa_out, 0, sizeof(*sa_out));
    (*sa_out).sa_handler = handler;
    sigaction(signal, sa_out, NULL);
}

//region ERROR HANDLING
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
 * global variables used: settings_g - To access the program name as specified by argv[0].
 *
 * @param additionalMessage The custom message to print along with the program name and errno contents.
 * @param line              The line number to print along with the error message.
 */
static inline void printErrnoAndTerminate(const char *additionalMessage, int line)
{
    fprintf(stderr, "ERROR in %s (line %d) - %s; ERRNO: %s\n",
            settings_g.programName, line, additionalMessage, strerror(errno));

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
    bool indexSpecified = false;

    while ((opt = getopt(argc, argv, "lp:i:")) != -1)
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

            case 'i':
                if (indexSpecified)
                    printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

                indexSpecified = true;
                settings_g.index = optarg;
                break;

            default:
                printUsageErrorAndTerminate(ERROR_UNKNOWN_ARGUMENT);
        }
    }

    if (optind != argc - 1)
        printUsageErrorAndTerminate(ERROR_INVALID_ARGUMENT_NUMBER);

    if (portSpecified == false)
        settings_g.port = DEFAULT_PORT;

    if (indexSpecified == false)
        settings_g.index = DEFAULT_INDEX;

    settings_g.root = argv[optind];
}
//endregion

//region HTTP CLIENT
/**
 * @brief Tries to create & bind a socket and then starts listening for incoming requests.
 * @details Terminates the program with EXIT_FAILURE upon failure of any called method.
 *
 * Global variables used: settings_g
 *
 * @param socketFd_out A pointer to the file descriptor of the socket. Will be set by this function.
 */
static inline void tryStartListening(int *socketFd_out)
{
    struct addrinfo *addressInfo;
    struct addrinfo options = {
            .ai_flags    = ADDR_INFO_FLAGS,
            .ai_family   = SOCKET_DOMAIN,
            .ai_socktype = SOCKET_CONNECTION_TYPE,
            .ai_protocol = SOCKET_PROTOCOL
    };

    int result = getaddrinfo(NULL, settings_g.port, &options, &addressInfo);
    if (result != 0) {
        fprintf(stderr, "%s: %s\n", ERROR_GETTING_INFO, gai_strerror(result));
        freeaddrinfo(addressInfo);
        exit(EXIT_FAILURE);
    }

    TRY(*socketFd_out = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol),
        ERROR_SOCKET_CREATION);

    int optionValue = SOCKET_OPTION_VALUE;
    setsockopt(*socketFd_out, SOCKET_OPTION_LVL, SOCKET_OPTION_NAME, &optionValue, sizeof optionValue);

    TRY(bind(*socketFd_out, addressInfo->ai_addr, addressInfo->ai_addrlen), ERROR_BIND);
    TRY(listen(*socketFd_out, BACKLOG), ERROR_LISTENING);
}

/**
 * @brief Tries to write the specified data to the specified connection. Terminates the program with EXIT_FAILURE
 * upon failure.
 * @details Calls fflush on the connection after writing the data.
 *
 * @param connection A stream the data should be written to.
 * @param data       The data to write.
 */
static inline void trySend(FILE *connection, char *data)
{
    TRY(fputs(data, connection), ERROR_SEND_RESPONSE);
    TRY(fflush(connection), ERROR_SEND_RESPONSE);
}

/**
 * @brief Creates a response header with the specified status-code, status description and the protocol version as specified in HTTP_PROTOCOL,
 * adds the Header 'Connection: close' and writes the headers to the specified connection.
 * @details Terminates the program with EXIT_FAILURE upon failure of any called method.
 *
 * @param connection The connection to a client to write the headers to.
 * @param statusCode The status-code to set.
 * @param statusDesc THe status-description to set.
 */
static inline void trySendEmptyResponse(FILE *connection, char *statusCode, char *statusDesc) {
    char *response;
    TRY_PTR(response = calloc(1, 1), "calloc failed");

    tryCreateResponseHeader(HTTP_PROTOCOL, statusCode, statusDesc, &response);
    tryAddResponseHeader("Connection", "close", true, &response);
    trySend(connection, response);

    free(response);
}

/**
 * @brief Checks if the given request is valid and supported and answers the client if this is not the case.
 * Otherwise extracts the requested filepath and returns it.
 * @details If the first line of the request is not '<method> <path> HTTP_PROTOCOL' 400 Bad Request is written to
 * connection and NULL is returned. If <method> is not HTTP_GET 501 Not implemented is written to connection
 * and NULL is returned.
 * Terminates the program with EXIT_FAILURE upon failure of any called method.
 *
 * @param request    The request to parse & verify.
 * @param connection The connection to a client to write answers to in case of an invalid request.
 * @return The requested filepath (<path>).
 */
static inline char *tryProcessRequest(char *request, FILE *connection)
{
    // request is a copy of the pointer so strsep doesn't cause losing the reference for freeing later
    char *requestMethod = strsep(&request, " ");
    char *filePath = strsep(&request, " ");
    char *protocol = strsep(&request, "\r");

    if (requestMethod == NULL || filePath == NULL || protocol == NULL ||
        strcmp(protocol, HTTP_PROTOCOL) != 0 || request[0] != '\n')
    {
        trySendEmptyResponse(connection, "400", "Bad Request");
    }
    else if (strcmp(requestMethod, HTTP_GET) != 0) {
        trySendEmptyResponse(connection, "501", "Not implemented");
    }
    else
        return filePath;

    return NULL;
}

/**
 * @brief Checks if the given fileName ends with .html, .htm, .css or .js
 * and if so saves the corresponding MIME type in the location pointed to by type_out.
 * @details Terminates the program with EXIT_FAILURE upon failure of any called method.
 * type_out must be a pointer to an allocated memory area initialised with the empty string!
 *
 * @param fileName The filename to check.
 * @param type_out A pointer to where the type should be stored.
 * Must be a pointer to an allocated memory area initialised with the empty string.
 */
static inline void tryGetMimeType(char* fileName, char **type_out)
{
    if (strstr(fileName, ".html") != NULL || strstr(fileName, ".htm") != NULL)
        TRY_CONCAT(type_out, "text/html");

    else if (strstr(fileName, ".css") != NULL)
        TRY_CONCAT(type_out, "text/css");

    else if (strstr(fileName, ".js") != NULL)
        TRY_CONCAT(type_out, "application/javascript");
}

/**
 * @brief Tries to open settings_g.root/requestedFilePath[/INDEX if requestedFilePath ends with /]
 * and write the contents to connection.
 * @details If the file can't be opened '404 Not Found' is written, otherwise the response also includes the headers
 * Date & Content-Length.
 * Terminates the program with EXIT_FAILURE upon failure of any called method.
 *
 * @param connection        The stream to write the response to.
 * @param requestedFilePath The path of the requested resource relative to settings_g.root.
 */
static inline void trySendFile(FILE *connection, char *requestedFilePath)
{
    LOG("Requested file-path: %s\n", requestedFilePath);

    char *filePath;
    TRY_PTR(filePath = calloc(1, 1), "calloc failed");
    TRY_CONCAT(&filePath, settings_g.root, requestedFilePath, endsWith(requestedFilePath, '/') ?
                                                              settings_g.index : "");
    LOG("Resulting file-path: %s\n\n", filePath);

    FILE *requestedFile;
    if ((requestedFile = fopen(filePath, TARGET_FILE_OPTION)) == NULL)
        trySendEmptyResponse(connection, "404", "Not Found");
    else
    {
        char *response;
        TRY_PTR(response = calloc(1, 1), "calloc failed");
        tryCreateResponseHeader(HTTP_PROTOCOL, "200", "OK", &response);

        char date[MAX_RFC822_STRING];
        getDateInRFC822(&date);
        tryAddResponseHeader("Date", date, false, &response);

        char fileSizeStr[MAX_LONG_STRING];
        tryGetSize(requestedFile, &fileSizeStr);
        tryAddResponseHeader("Content-Length", fileSizeStr, false, &response);

        char *mimeType;
        TRY_PTR(mimeType = calloc(1,1), "calloc failed");
        tryGetMimeType(filePath, &mimeType);
        if (strcmp(mimeType, "") != 0)
            tryAddResponseHeader("Content-Type", mimeType, false, &response);

        tryAddResponseHeader("Connection", "close", true, &response);
        LOG("Response-Header:\n%s", response);
        trySend(connection, response);
        free(response);

        char *fileContent;
        TRY_PTR(fileContent = malloc(1), "malloc failed");
        tryReadFile(requestedFile, &fileContent);
        LOG("Response-Body:\n%s\n\n", fileContent);
        trySend(connection, fileContent);

        free(mimeType);
        free(fileContent);
        fclose(requestedFile);
    }

    free(filePath);
}
//endregion

//region UTILITY
/**
 * @brief Writes the current UTC time in RFC822 format to the array pointed to by date_out.
 * @param date_out A pointer to an array in which the result is to be stored.
 */
static inline void getDateInRFC822(char (*date_out)[MAX_RFC822_STRING])
{
    time_t localTime;
    struct tm *utcTimePtr;

    time(&localTime);
    utcTimePtr = gmtime(&localTime);

    strftime(*date_out, MAX_RFC822_STRING, "%a, %d %b %y %H:%M:%S %Z", utcTimePtr);
}

//region HTTP
/**
 * @brief Creates a Response-Header in the form "<protocol> <statusCode> <statusDesc>\r\n", eg "HTTP/1.1 200 OK".
 * @details Terminates the program with EXIT_FAILURE if concatenating fails.
 *
 * @param protocol     The protocol used for communication.
 * @param statusCode   The status code of the response eg 200.
 * @param statusDesc   The status description of the response eg 'OK'.
 * @param response_out A pointer to where the response should be stored. Must be an allocated, 0 initialized address-space.
 */
static inline void tryCreateResponseHeader(char *protocol, char *statusCode, char *statusDesc, char **response_out) {
    TRY_CONCAT(response_out, protocol, " ", statusCode, " ", statusDesc, "\r\n");
}

/**
 * @brief Adds a Header in the form "<header>: <value>\r\n" to a given GET-Request.
 * If last is true an additional '\r\n' is added to indicate completion of the header-section.
 * @details Reallocates request_out. Terminates the program with EXIT_FAILURE if concatenating fails.
 *
 * @param header       The name of the header.
 * @param value        The value of the header.
 * @param last         Indicates if this header finishes the header-section.
 * @param response_out A pointer to where the result should be stored. Must be an allocated, 0 initialized address-space.
 */
static inline void tryAddResponseHeader(char *header, char *value, bool last, char **response_out) {
    TRY_CONCAT(response_out, header, ": ", value, "\r\n", last ? "\r\n" : "");
}
//endregion

//region STREAMS
/**
 * @brief Determines the size in bytes of the given file and saves the result as string in the array pointed to by size_out.
 * @details
 * @param file
 * @param size_out
 */
static inline void tryGetSize(FILE *file, char (*size_out)[MAX_LONG_STRING])
{
    long fileSize;
    TRY(fseek(file, 0, SEEK_END), ERROR_DETERMINING_FILESIZE);
    TRY(fileSize = ftell(file), ERROR_DETERMINING_FILESIZE);
    TRY(fseek(file, 0, SEEK_SET), ERROR_DETERMINING_FILESIZE); //Go back to beginning of file
    TRY(snprintf(*size_out, MAX_LONG_STRING, "%ld", fileSize), ERROR_DETERMINING_FILESIZE);
}

/**
 * @brief Reads from the specified file until EOF or is read and stores the result in content_out.
 * @details content_out must be a pointer to an allocated address-space. Will be reallocated.
 * If reallocating content_out fails, the program calls printErrnoAndTerminate
 * and thereby terminates the program with EXIT_FAILURE.
 *
 * @param file        The file to read.
 * @param content_out A pointer to where the read data should be stored. Must be an allocated address-space.
 */
static inline void tryReadFile(FILE *file, char **content_out)
{
    int i = 0;
    for (int character; (character = fgetc(file)) != EOF; i++)
    {
        TRY_PTR(*content_out = realloc(*content_out, i + 2), "realloc failed.");
        (*content_out)[i] = (char) character;
    }
    (*content_out)[i] = '\0';
}

/**
 * @brief Reads from the specified connection-stream until EOF or '\r\n\r\n' is read and stores the result in request_out.
 * @details request_out must be a pointer to an allocated, 0 initialized address-space. Will be reallocated.
 * If reallocating request_out fails, the program calls printErrnoAndTerminate
 * and thereby terminates the program with EXIT_FAILURE.
 *
 * @param connection  The stream to the client.
 * @param request_out A pointer to where the result should be stored. Must be an allocated, 0 initialized address-space.
 */
static inline void tryReadRequest(FILE *connection, char **request_out)
{
    int character;
    for (int i = 0; (character = fgetc(connection)) != EOF; i++)
    {
        TRY_PTR(*request_out = realloc(*request_out, i + 2), "realloc failed.");
        (*request_out)[i] = (char) character;
        (*request_out)[i + 1] = '\0';

        if (i >= 3 && strcmp(&((*request_out)[i - 3]), "\r\n\r\n") == 0)
            break;
    }
}
//endregion

//region STRINGS
/**
 * @brief Determines whether a given string ends with a given character or not.
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