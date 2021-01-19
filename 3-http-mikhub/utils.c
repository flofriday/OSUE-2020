#include "utils.h"

long parse_port(char* port_arg) {
    char *endptr;
    long port = strtol(port_arg, &endptr, 10);
    if (endptr == port_arg) {
        fprintf(stderr, "[%s]: Invalid port number ('%s' is not a number)\n", PROGRAM_NAME, port_arg);
        USAGE();
    }
    if (port == LONG_MIN || port == LONG_MAX) {
        ERROR_EXIT("Overflow occurred while parsing port number", strerror(errno));
    }
    if (port < 0) {
        fprintf(stderr, "[%s]: Negative port number %ld not allowed\n", PROGRAM_NAME, port);
        USAGE();
    }

    return port;
}

void LOG(char* format, ...) {
    va_list args;

    char date_time[256];
    time_t tictoc;
    struct tm *now;
    time(&tictoc);
    now = localtime(&tictoc);
    strftime(date_time, 256, "[%e.%m.%y|%H:%M:%S]", now);

    va_start(args, format);    
    printf("%s(%s) ", date_time, PROGRAM_NAME);
    vfprintf(stdout, format, args);
    printf("\n");
    va_end(args);
}

void ERROR_LOG(char *message, char *error_details) {
    if (error_details == NULL) {
        LOG("%s", PROGRAM_NAME, message);
    } else {
        LOG("%s (%s)", PROGRAM_NAME, message, error_details);
    }
}

void ERROR_EXIT(char *message, char *error_details) {
    ERROR_MSG(message, error_details);
    exit(EXIT_FAILURE);
}
void ERROR_MSG(char *message, char *error_details) {
    if (error_details == NULL) {
        fprintf(stderr, "[%s]: %s\n", PROGRAM_NAME, message);
    } else {
        fprintf(stderr, "[%s]: %s (%s)\n", PROGRAM_NAME, message, error_details);
    }
}
void USAGE() {
    fprintf(stderr, USAGE_MESSAGE, PROGRAM_NAME);
    exit(EXIT_FAILURE);
}