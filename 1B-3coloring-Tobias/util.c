/**
 * @file   util.h
 * @author Tobias de Vries (e01525369)
 * @date   20.11.2020
 *
 * @brief Implements the convenience functions specified in util.h.
 **/

#include "util.h"


/** For documentation see util.h */
void printErrorAndTerminate(const char *message)
{
    fprintf(stderr, CUSTOM_ERROR_FORMAT, programName_g, message);
    exit(EXIT_FAILURE);
}

/** For documentation see util.h */
void printErrnoAndTerminate(const char *additionalMessage)
{
    fprintf(stderr, ERRNO_ERROR_FORMAT, programName_g, additionalMessage, strerror(errno));
    exit(EXIT_FAILURE);
}

/** For documentation see util.h */
void exitOnFailure(int result, const char *additionalMessage)
{
    if (result == -1)
        printErrnoAndTerminate(additionalMessage);
}
