/**
 * @file error.h
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Error printing
 *
 * This module defines a function to print an error to stderr
 **/

#pragma once

#include <stdarg.h>
#include <stdio.h>

extern const char *program_name; /**< The program name. Needs to be defined by the application. **/

/**
 * @brief Prints an error message to stderr. The error message is prefixed with the program name (program_name) enclosed in square brackets.
 * @param format_string The format string
 * @param ... Format arguments
 */
__attribute__((format (printf, 1, 2))) void error(const char *const restrict format_string, ...)
{
	va_list args;
	va_start(args, format_string);
	fprintf(stderr, "[%s] ", program_name);
	vfprintf(stderr, format_string, args);
	va_end(args);

	fprintf(stderr, "\n");
}
