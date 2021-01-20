/**
 * 
 * @file mycompress.c
 * @author briemelchen
 * @date 30 Oct 2020
 * @brief mycompress reads strings, compresses them and writes them to an output. Statistic will be printed to sterr.
 * 
 * @details mycompress takes as option -o outputfile, where outputfile is a  file, if no output file is given, it will  print to stdout. 
 * Furthermore, the programm takes a arbitrarily number of input files as arguments. If no input files are given, mycompress reads from stdin.
 * Foreach inputfile given, mycompress reads them and compresses them line-by-line, after compressing, they are written to the given output-file (or stdout).
 * The compress process runs as followed: the input is compressed by subsituting subsequent identical characters by only one occurence of the character followed
 * by the number of characters(e.g aaabb => a3b2). This process also handles new-line characters. After the compressing, the compressed lines are printed
 * to the output file. Statistics containing the number of written/read characters as well as a compression ratio are printed to sterr.
 * 
 * 
 */

#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <errno.h>

#include <string.h>

#include <string.h>

#define PROGRAM_NAME "mycompress"

/**
 * @brief reads from an input file, compresses it and writes the compressed lines to an output file.
 * @details compress_and_print reads from an input in file compresses it (via function compress(..)) and writes it to the outputfile out.
 * Foreach line compress(..) gets called. The input file gets closed after processing it. The output file is not and has to be closed by callers of compress_and_print.
 * Additional, the function calls has_new_line_chars for each line, to check if there are newline- characters which need to be processed.
 * If any error occurs, the error is written to sterr and the program exits. Uses global variable final_stats to update the statistics.
 * @param in which should be read and compressed. In is closed after processing.
 * @param out which the compressed string is written to. Out is NOT closed after processing. 
 * 
 * 
 */
static void compress_and_print(FILE *in, FILE *out);

/**
 * @brief compress takes an char-pointer aswell as integerpointer. The string is getting compressed as specified. The compressed string is returned.
 * @details compress takes an charpointer and iterates through it char by char (until terminating byte is reached). The string 
 * is compressed by substituting subsequent identical characters by only one occurrence of the character followed by the number of characters(e.g aaabb => a3b2)
 * The additional parameter new_line_chars holds the address of an integer, where the numbers of preceded newline characters (\n) is saved. 
 * (e.g. if 2 newlines preeceded the line aabb => 2a2b2)
 * When error occurs (e.g. memory allocation fails) the program exits. Uses global variable final_stats to update the statistics.
 * @param toCompress pointer to the char(s) which should be compressed.
 * @param new_line_chars holds the address of the count of preceded newline characters
 * @return the compressed string in the specified format.
 */
static char *compress(char *toCompress, int *new_line_chars);

/**
 * @brief checks if a given char-pointer (which has to be \0 terminated), contains a new line character '\n'
 * @details takes a string and checks if it contains a new line character '\n'. Returns 0 if it contains one, otherwise -1.
 * @param str string to be checked. Has to be NUL-terminated ('\0')
 * @return 0 if the string contains a new line character, otherwise -1
 */
static int has_new_line_char(char *str);

/**
 * @brief prints the usage message to stderr of the program and exits.
 * @details prints the usage message in format "USAGE: PROGRAM_NAME options arguments"  on stderr and exits the program.
 */
static void usage(void);

/**
 * @brief prints an error message to stderr and exits the program.
 * @details prints an given error message in format  "[PROGRAM_NAME] ERROR: reasons" to stderr and exits the program afterwards.
 * @param error_message custom message to be printed.
 */
static void error(char *error_message);

typedef struct
{
    long int read_chars;
    long int written_chars;
} stats;

stats final_stats = {0, 0}; // updates always when chars are written/read

/**
 * @brief starting point of the program. Parses options and arguments and open(s) files and calls processing functions.
 * @details entry point of the program. Parses the arguments and options of the programm. Opens the in/output files and 
 * calls functions to proceed them. Uses global variable final_stats to update the statistics.
 * @param argc counter of arguments
 * @param argv holding the program's arguments and options.
 */
int main(int argc, char **argv)
{
    int opt_o = 0;
    char c;
    FILE *out;
    while ((c = getopt(argc, argv, "o:")) != -1)
    {
        switch (c)
        {
        case 'o':
            opt_o++;
            if (opt_o >= 2)
            {
                if (fclose(out) != 0)
                {
                    error("fclose failed");
                }
                usage();
                break;
            }

            if ((out = fopen(optarg, "w")) == NULL)
            {
                error("Failed to open outputfile");
            }
            break;
        default: // all other options except  -o
            usage();
            break;
        }
    }

    if (opt_o == 0)
    {
        out = stdout;
    }
    else if (opt_o >= 2)
    {
        if (fclose(out) != 0)
        {
            error("fclose failed");
        }
        error("Maximum one output file possible");
    }

    int arg_index = optind;

    if (argv[arg_index] == NULL)
    {
        FILE *in = stdin;
        compress_and_print(in, out);
        if (fclose(out) != 0)
        {
            error("fclose failed");
        }
    }
    else
    {
        while (argv[arg_index] != NULL)
        {
            FILE *in;
            if ((in = fopen(argv[arg_index], "r")) == NULL)
            {
                error("Could not open file: fopen failed");
            }
            else
            {
                compress_and_print(in, out);
            }

            arg_index++;
        }

        if (fclose(out) != 0)
        {
            error("fclose failed");
        }
    }
    fprintf(stderr, "Read:\t\t\t%li characters\n", final_stats.read_chars);
    fprintf(stderr, "Written:\t\t%li characters\n", final_stats.written_chars);
    fprintf(stderr, "Compression ratio:\t%.1f%%\n", ((float)final_stats.written_chars) / ((float)final_stats.read_chars) * 100);
}

static void compress_and_print(FILE *in, FILE *out)
{
    char *line = NULL;
    size_t n = 0;
    ssize_t result;
    int new_line_chars = 0;

    while ((result = getline(&line, &n, in)) != -1)
    {
        char *compressed = compress(line, &new_line_chars);
        if (fputs(compressed, out) < 0)
        {
            free(line);
            error("Writing to stream failed: fputs failed");
        }

        if (has_new_line_char(line) == 0)
        {
            new_line_chars++;
        }
        free(compressed);
        final_stats.read_chars += result; // update statistics
    }

    free(line);

    if (new_line_chars != 0)
    {
        int bytes_needed = snprintf(NULL, 0, "%d", new_line_chars);
        char *buffer = malloc(bytes_needed + 2);
        snprintf(buffer, bytes_needed + 2, "%d", new_line_chars);
        strncat(buffer, "\n", 2);
        final_stats.written_chars += 1;
        if (fputs(buffer, out) < 0)
        {
            error("Writing to stream failed: fputs failed");
        }
        free(buffer);
    }

    if (ferror(in) != 0)
    {
        error("Reading from File failed");
    }

    if (fclose(in) != 0)
    {
        error("fclose failed");
    }
}

static char *compress(char *toCompress, int *new_line_chars)
{

    int needed_bytes_newlines = snprintf(NULL, 0, "%d", *new_line_chars); // size needed for storing the new-line-char

    // max size is size of input * 2 if no consecutive chars are given (e.g.: "abc" -> "a1b1c1") + newline-char + null byte + bytes needed for preceding newline chars
    char *compressed = malloc(sizeof(char) * strlen(toCompress) * 2 + 2 + needed_bytes_newlines);
    long int count = 0;
    strcpy(compressed, ""); //init empty string
    if (compressed == NULL)
    {
        error("Memory allocation failed");
    }

    if (*toCompress == '\n') // if line is empty
    {
        return compressed;
    }
    else if (*new_line_chars != 0) // adds preceding newline-characters
    {
        char *buffer = malloc(needed_bytes_newlines + 1);
        snprintf(buffer, needed_bytes_newlines + 1, "%d", *new_line_chars);
        strncat(compressed, buffer, needed_bytes_newlines);
        final_stats.written_chars += strlen(buffer);
        free(buffer);
        *new_line_chars = 0;
    }

    while (*toCompress != '\0')
    {
        char ac_char = *toCompress;
       
        while (*toCompress == ac_char)
        {
            count++;
            toCompress++;
        }

        if (ac_char != '\n')
        {
            int bytes_needed = snprintf(NULL, 0, "%ld", count); // get buffer size for the count
            char *buffer = malloc(bytes_needed + 1);
            snprintf(buffer, bytes_needed + 1, "%ld", count);

            if (buffer == NULL)
            {
                error("Memory allocation failed");
            }

            strncat(compressed, &ac_char, 1);
            strncat(compressed, buffer, bytes_needed);
            final_stats.written_chars += strlen(buffer) + 1;
            free(buffer);
        }
        count = 0;
     
    }
    strcat(compressed, "\n");
    final_stats.written_chars++;
    return compressed;
}

static int has_new_line_char(char *str)
{
    while (*str != '\0')
    {
        if (*str == '\n')
        {
            return 0;
        }
        str++;
    }
    return -1;
}

static void error(char *error_message)
{
    fprintf(stderr, "[%s] ERROR: %s: %s.\n", PROGRAM_NAME, error_message, strcmp(strerror(errno), "Success") == 0 ? "Failure" : strerror(errno));
    exit(EXIT_FAILURE);
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s [-o outfile] [file...]\n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}