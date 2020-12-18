/**
 * @file mygrep.c
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief mygrep
 *
 * This program greps one or more input files (both case sensitive and insensitive) for a keyword and
 * outputs the matching lines to stdout or another output file.
 **/

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char *program_name; /**< The program name. Needs to be defined by the application. **/

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


/**
 * @brief Writes up to length lowercase equivalents of the chars in input to output
 * @param input The input buffer
 * @param output The output buffer
 * @param length The number of chars to write
 */
static void lowercase(const char *const restrict input, char *const restrict output, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		output[i] = (char) tolower((unsigned char) input[i]);
	}
}

/**
 * @brief Greps a file for a keyword and outputs the matching lines to stdout.
 * @param input An input file opened for reading
 * @param keyword The keyword to search for
 * @param output An output file opened for writing to write the matching lines to
 * @param case_insensitive Whether the search should occur without regard to case sensitivity
 * @return EXIT_SUCCESS on success
 * @return EXIT_FAILURE on failure. An error message has been written to stderr
 */
int mygrep(FILE *input, const char *const keyword, FILE *output, bool case_insensitive)
{
	int ret = EXIT_SUCCESS;
	
	char *keyword_buffer;
	char *haystack = NULL;
	size_t length;
	char *haystack_buffer = NULL;
	
	if (case_insensitive)
	{
		length = strlen(keyword);
		
		keyword_buffer = malloc(length * sizeof(char));
		if (keyword_buffer == NULL)
		{
			error("malloc failed");
			ret = EXIT_FAILURE;
			goto cleanup;
		}

		lowercase(keyword, keyword_buffer, length);
	}
	else
	{
		/* This const cast is needed, as keyword_buffer can't be const
		   due to it being passed to lowercase in another branch */
		keyword_buffer = (char *const) keyword;
	}

	length = 0;
	while ((getline(&haystack, &length, input)) != -1)
	{
		if (case_insensitive)
		{
			if (!(haystack_buffer = realloc(haystack_buffer, length * sizeof(char))))
			{
				error("realloc failed: %s", strerror(errno));
				ret = EXIT_FAILURE;
				goto cleanup;
			}
			lowercase(haystack, haystack_buffer, length);
		}
		else
		{
			haystack_buffer = haystack;
		}
		
		if (strstr(haystack_buffer, keyword_buffer))
		{
			fprintf(output, "%s", haystack);
		}
	}

cleanup:
	if (case_insensitive)
	{
		free(haystack_buffer);
		free(keyword_buffer);
	}

	free(haystack);

	return ret;
}

/**
 * @brief main
 * The main program. It first reads all options via getopt and complains about missing ones.
 * It then scans the remaining arguments for input files to search in if none are passed, stdin is used.
 * @param argc The argument count
 * @param argv The arguments
 * @return EXIT_SUCCESS on success
 * @return EXIT_FAILURE on failure with an error message written to stderr
 */

int main(int argc, char **argv)
{
	program_name = argv[0];

	bool case_insensitive = false;
	FILE *output_file = stdout;
	
	int c;
	while ((c = getopt(argc, argv, "i::o:")) != -1)
	{
		switch (c)
		{
		case 'i':
			case_insensitive = true;
			break;

	    case 'o':
		{
			FILE *f = fopen(optarg, "w");
			if (f)
			{
				output_file = f;
			}
			else
			{
				error("Cannot open output file %s: %s", optarg, strerror(errno));
				return EXIT_FAILURE;
			}

			break;
		}

		case '?':
		default:
			return EXIT_FAILURE;
		}
	}

	int ret = EXIT_SUCCESS;
	int diff;

	char *keyword;
	if (optind < argc && strlen(argv[optind]) > 0)
	{
		keyword = argv[optind++];
	}
	else
	{
		error("Parameter keyword required.");

		fprintf(stderr, "Usage: %s [-i] [-o outfile] keyword [file...]\n", argv[0]);
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	diff = argc - optind;

	if (diff > 0)
	{
		for (int pos = optind; pos < argc; ++pos)
		{
			FILE *f = fopen(argv[pos], "r");
			if (f)
			{
				ret = mygrep(f, keyword, output_file, case_insensitive);
				fclose(f);
			}
			else
			{
				error("Failed to read input file %s", argv[pos]);
				ret = EXIT_FAILURE;
			}
		}
	}
	else if (diff == 0)
	{
		ret = mygrep(stdin, keyword, output_file, case_insensitive);
	}
	else
	{
		error("optind is bigger than argc. This should not happen. Aborting");
		abort();
	}

cleanup:
	if (output_file != stdout)
	{
		fclose(output_file);
	}

	return ret;
}
