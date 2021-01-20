/**
 * @author briemelechen
 * @date 04.01.2021
 * @brief module contains some functionality, which does not contain to a 
 * specific module (client/server)
 * @details e.g. functionalities to check ports and urls on validity are 
 * provided by the module.
 * For implementation-details @see util.c
 * */

#ifndef util_h
#define util_h
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <zlib.h>

/**
 * @brief checks if a given port is valid
 * @details checks that the port only contains numbers
 * and nothing else.
 * @param port which should be checked
 * @return true if the port is valid, otherwise false
 **/ 
bool is_valid_port(const char *port);

/**
 * @brief checks if a given URL is a valid http-url
 * @details it is checked, if the URL starts with "http://",
 * to indicate a http conversation. 
 * @param URL to be checked
 * @return true if it is valid on the specified criteria, otherwise false
 **/
bool is_valid_url(const char *URL);

/**
 * @brief extracts the host and path out of a URL
 * @details e.g. http://test.com/asdf/ => host= test.com , path= /asdf/ 
 * @param URL the URL where the path and host should be extracted. 
 * The URL has to be checked in advanve on validty.
 * @param path pointer to a memory where the path can be saved.
 * Needs to be allocated in davance!
 * @param host pointer to a memory where the host can be saved.
 * Needs to be allocated in davance!
 * @return 0 on success, -1 on failure
 **/
int extract_host(const char *URL, char *path, char *host);

/**
 * @brief checks if a given response-header is valid and stores the information
 * @details a valid header contains the version (HTTP/1.1)
 * followed by a response code (e.g. 200) and description of the
 * response code. All values are stored in the passed pointers.
 * @param line of the header which should be checked (first line of response)
 * @param res pre-allocated pointer where
 *   the response code  + description are going to be stored
 * @param response_code integer where the response code is getting stored
 * @return true, if the header is valid, false in case of an error or invalidty
 **/
bool is_header_valid(char *line, char *res, int *response_code);

/**
 * @brief concats the file-name to a specific path (used for d-option in client)
 * @details if the file is not specified "/" at the end of the path, index.html is added
 * as path. (default value)
 * Otherwise the file name is concatinated.
 * @param d_opt_path pointer to a pointer where the full path, including file name
 * should be stored. Must be freed afterwards!
 * @param path path with ends with the file name which is needed or "/" if no is
 * specified
 * @return  0 on success, -1 on failure
 **/
int get_file_name(char **d_opt_path, const char *path);

/** 
 * @brief gets the file size (for content-length) of a specfic file
 * @details sets the indicator too the end (rewind afterwards) and uses ftell
 * to get the size
 * @param file where the size should be calculated
 * @return the size, or -1 on failure
 **/
int get_file_size(FILE *file);

/**
 * @brief returns the string representation of the mime-type of a file
 * @details checks for the ending of the file and maps it to a mimetype
 * If the mime-type is not supported, NULL is returned.
 * @param path with ends with the filename 
 * @return the text-representation of the mime-type (e.g. "text/html"), ór NULL in case of 
 * error or if the mime-type is not known
 **/
char* get_mime_type(char *full_path);

/**
 * @brief prints an error message and exits the program.
 * @details exits with exit code 1
 * @param error_message which should be printed
 * @param add_error_msg additional error message e.g. strerror(errno) which should be printed
 * @param prog_name calling the function 
 **/
void error(const char *error_message, const char *add_error_msg, char *prog_name);

/**
 * @brief prints an error message 
 * @param error_message which should be printed
 * @param add_error_msg additional error message e.g. strerror(errno) which should be printed
 * @param prog_name calling the function 
 **/
void error_m(const char *error_message, const char *add_error_msg, char *prog_name);

#endif