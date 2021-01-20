#ifndef _UTILS_H_
#define _UTILS_H_

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
#include <time.h>
#include <stdarg.h>

extern char* PROGRAM_NAME;
extern char* USAGE_MESSAGE;

/**
 * @brief Takes a string and parses it to a long
 * @details This function terminates the program if the number is negative, invalid or if 
 * there is an overflow
 */
long parse_port(char* port_arg);

// utils
void LOG(char* format, ...);
void ERROR_LOG(char* message, char* error_details);
void ERROR_EXIT(char* message, char* error_details);
void ERROR_MSG(char* message, char* error_details);
void USAGE();

#endif