/**
 * @file   util.h
 * @author Tobias de Vries (e01525369)
 * @date   20.11.2020
 *
 * @brief Provides convenience types and functions used in different files.
 **/

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>

#define ERRNO_ERROR_FORMAT  "%s - %s: %s\n"
#define CUSTOM_ERROR_FORMAT "%s - %s\n"

/** Convenience type to emulate a boolean value. */
typedef enum {
    false = 0,
    true = 1
} bool;

/** Must be specified by files using this header.
 * Should contain the program name as specified in argumentValues[0] */
extern char *programName_g;


/**
 * @brief Convenience function for -1 checks. Calls printErrnoAndTerminate if result equals -1.
 *
 * @param result            If this equals -1 printErrnoAndTerminate is called otherwise nothing happens
 * @param additionalMessage The message to pass to printErrnoAndTerminate if needed.
 */
void exitOnFailure(int result, const char *additionalMessage);

/**
 * @brief Prints the program name as specified in programName_g, a given message and the current content of errno
 * to stderr and terminates the program with EXIT_FAILURE.
 * @details global variables used: programName_g - The program name as specified in argumentValues[0]
 *
 * @param additionalMessage The custom message to print along with the program name and errno contents.
 */
void printErrnoAndTerminate(const char *additionalMessage);

/**
 * @brief like printErrnoAndTerminate but without printing the contents of errno.
 * @details global variables used: programName_g - The program name as specified in argumentValues[0]
 *
 * @param message The custom message to print along with the program name.
 */
void printErrorAndTerminate(const char *message);

#endif //UTIL_H
