/**
 * @file mygrep.c
 * @author Jonny X <e12345678@student.tuwien.ac.at>
 * @date 30.10.2020
 *
 * @brief This program takes keyword and optional input files and one optional output file as arguments.
 * The optional parameter i speccifies wheter it should ignore the case sensitivity of the keyword. 
 * It searches the input files (or stdin if no file is specified) and prints all lines which contain the keyword
 * to the specified output file (or stdout if no ouput file is specified).
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

char *myprog;

/**
 * @brief
 * Prints the usage format to explain which arguments are expected.
 * 
 * @details
 * Gets called when an input error by the user is detected.
**/
void usage(void)
{
    fprintf(stderr, "Usage: %s [-i] [-o outfile] keyword [file...]\n", myprog);
    exit(EXIT_FAILURE);
}

/**
 * Converts a char* to lower case.
 * @brief
 * This function converts the input to lower case.
 * 
 * @details
 * Iterates through each character and changes it to lower case with the help of tolower()
 * 
 * @param word the constant pointer to the input word which should be modified.
**/
void stringToLower(char *const word)
{
    for (size_t i = 0; word[i]; ++i)
    {
        word[i] = tolower(word[i]);
    }
}

/**
 * Program entry point.
 * @brief The program first parses the arguments. Then it iterates over each file 
 * and each line and searches for the keyword. Once a keyword is found it is printed
 * to the output.
 *  
 * @details Each input file is opened and read in the order they are given. The output 
 * file is only opened once. At the beginning myprog is set. This is a global char pointer 
 * which holds the program name. If the arguments specified contain an error the function
 * usage is called.
 * 
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char **argv)
{
    myprog = argv[0];

    bool ignore_case_flag = false;
    char *output_file_arg = NULL;
    int c;

    while ((c = getopt(argc, argv, "io:")) != -1)
    {
        switch (c)
        {
        case 'o':
            if (output_file_arg != NULL)
            {
                usage();
            }
            output_file_arg = optarg;
            break;
        case 'i':
            if (ignore_case_flag == true)
            {
                usage();
            }
            ignore_case_flag = true;
            break;
        case '?':
            usage();
            break;
        default:
            //should be unreachable
            assert(0);
            break;
        }
    }

    char *keyword = argv[optind];

    if (keyword == NULL)
    {
        usage();
    }

    char *input_file_name = argv[optind + 1];

    FILE *input_file = stdin;
    char *buffer = NULL;
    size_t buffer_length = 0;
    ssize_t read_length;

    FILE *output_file = stdout;

    if (output_file_arg != NULL)
    {
        if ((output_file = fopen(output_file_arg, "w")) == NULL)
        {
            fprintf(stderr, "[%s] Error: fopen with output-file %s failed: %s\n", myprog, output_file_arg, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 1; ((input_file_name = argv[optind + i]) != NULL) || input_file == stdin; ++i)
    {

        if (input_file_name != NULL)
        {
            if ((input_file = fopen(input_file_name, "r")) == NULL)
            {
                fprintf(stderr, "[%s] Error: fopen with input-file %s failed: %s\n", myprog, input_file_name, strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        while ((read_length = getline(&buffer, &buffer_length, input_file)) != -1)
        {

            char buffer_copy[read_length];
            strcpy(buffer_copy, buffer);

            if (ignore_case_flag == true)
            {
                stringToLower(buffer);
                stringToLower(keyword);
            }

            //keyword was found
            if (strstr(buffer, keyword) != NULL)
            {
                if (fputs(buffer_copy, output_file) == EOF)
                {
                    fprintf(stderr, "[%s] Error: fputs failed: %s\n", myprog, strerror(errno));
                    free(buffer);
                    exit(EXIT_FAILURE);
                }
            }
        }

        if (input_file != stdin)
        {
            if (fclose(input_file) == EOF)
            {
                //dont exit just because fclose fails
                fprintf(stderr, "[%s] Error: fclose failed: %s. Program continues.\n", myprog, strerror(errno));
            }
        }
    }

    // if the output_file is not stdout -> close the file
    if (output_file != stdout)
    {
        if (fclose(output_file) == EOF)
        {
            //dont exit just because fclose fails
            fprintf(stderr, "[%s] Error: fclose failed: %s. Program continues.\n", myprog, strerror(errno));
        }
    }

    //needs to be freed, because getline() allocates memory
    if (buffer != NULL)
    {
        free(buffer);
    }

    exit(EXIT_SUCCESS);
}