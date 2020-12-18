/**
 * @file main.c
 * @author flofriday XXXXXXXX <eXXXXXXXX@student.tuwien.ac.at>
 * @date 21.10.2020
 *
 * @brief Main program module, implements the full functionallity of mycompress.
 **/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

/**
 * Usage function.
 * @brief This function writes a short usage description to stderr and exits the
 * program.
 * @details Exit the process with EXIT_FAILURE.
 * @param prog_name The name of the program.
 **/
void usage(const char prog_name[])
{
    fprintf(stderr, "[%s] Usage: %s [-o outfile] [file...]\n", prog_name, prog_name);
    exit(EXIT_FAILURE);
}

/**
 * Compression function.
 * @brief This function reads from the in file compresses the data and writes 
 * the compressed characters to the out file. It also increases the counter read
 * and written accourding to the characters it read and wrote.
 * @details The function asumes all provided pointers are vaild and will
 * produce a segmentation vault if they are not.
 * @param in The file to compress and to read from.
 * @param out The file to write to.
 * @param read Pointer to the read characters counter.
 * @param written Pointer to the written characters pointer.
 * @return Upon success 0, or -1 if writing to the out file failed.
 */
int compress(FILE *in, FILE *out, uint64_t *read, uint64_t *written)
{
    int cnt = 0; // counts how often a char is repeated
    int last;
    while (true)
    {
        //  Read the current char and exit if EOF is reached
        int c = fgetc(in);
        if (feof(in))
        {
            break;
        }
        *read = *read + 1;

        // The first character
        if (cnt == 0)
        {
            cnt = 1;
            last = c;
            continue;
        }

        // If the current character is equal to the last
        if (c == last)
        {
            cnt++;
            continue;
        }

        // The current character is unequal to the last
        // Write the compressed data to the outfile.
        int tmp = fprintf(out, "%c%d", last, cnt);
        if (tmp == -1)
        {
            return -1;
        }
        *written += tmp;

        // Reset for the next chracter
        last = c;
        cnt = 1;
    }

    // Write the last character to the file
    int tmp = fprintf(out, "%c%d", last, cnt);
    if (tmp == -1)
    {
        return -1;
    }
    *written += tmp;
    return 0;
}

/**
 * Program entry point
 * @brief This function is the entrypoint of the mycompress program. This
 * function implements the arguent parsing, file opening and closing and 
 * generating the statistics. Only the compression algorithm is implemented 
 * in another function.
 * @param argc
 * @param argv
 * @return Upon success EXIT_SUCCESS is returned, otherwise EXIT_FAILURE.
 **/
int main(int argc, char *argv[])
{
    // Read the flags with getopts and save the output file into out_filename
    const char *out_filename = NULL;
    int c;
    const char *const progname = argv[0];
    while ((c = getopt(argc, argv, "o:")) != -1)
    {
        switch (c)
        {
        case 'o':
            if (out_filename != NULL)
            {
                fprintf(stderr, "[%s] ERROR: flag -o can only appear once\n", progname);
                usage(progname);
            }
            out_filename = optarg;
            break;
        case '?':
            // invalid option
            usage(progname);
            break;
        default:
            // unreachable code
            assert(0);
        }
    }

    // Save the input files into inputfilenames
    int input_len = argc - optind;
    char const *input_filenames[input_len];
    for (int i = 0; i < input_len; i++)
    {
        input_filenames[i] = argv[optind + i];
    }

    // Open the outputfile
    FILE *out_file = stdout;
    if (out_filename != NULL)
    {
        out_file = fopen(out_filename, "w");
        if (out_file == NULL)
        {
            fprintf(stderr, "[%s] ERROR: opening file %s failed: %s\n", argv[0], out_filename, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // Compress the files
    uint64_t read = 0;
    uint64_t written = 0;
    for (int i = 0; i < input_len; i++)
    {
        FILE *in_file = fopen(input_filenames[i], "r");
        if (in_file == NULL)
        {
            fprintf(stderr, "[%s] ERROR: opening file %s failed: %s\n", argv[0], input_filenames[i], strerror(errno));
            fclose(out_file);
            exit(EXIT_FAILURE);
        }
        if (compress(in_file, out_file, &read, &written) == -1)
        {
            fprintf(stderr, "[%s] ERROR: An error occoured while compressing file %s: %s\n", argv[0], input_filenames[i], strerror(errno));
            fclose(in_file);
            fclose(out_file);
            exit(EXIT_FAILURE);
        }
        fclose(in_file);
    }

    // If no input file was specified use stdin
    if (input_len == 0)
    {
        if (compress(stdin, out_file, &read, &written) == -1)
        {
            fprintf(stderr, "[%s] ERROR: An error occoured while compressing file stdin: %s\n", argv[0], strerror(errno));
            fclose(out_file);
            exit(EXIT_FAILURE);
        }
    }

    // Close the output file if it wasn't stdout
    if (out_file != stdout)
    {
        fclose(out_file);
    }

    // Print the statistics
    fprintf(stderr, "Read: %7lu characters\nWritten: %4lu chararcets\nCompression ratio: %4.1f%%\n", read, written, ((double)(written) / read) * 100.0);
    return EXIT_SUCCESS;
}
