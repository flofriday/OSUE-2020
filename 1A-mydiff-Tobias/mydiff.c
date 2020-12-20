/**
 * @file   main.c
 * @author Tobias de Vries (e01525369)
 * @date   02.11.2020
 *
 * @brief A program to compare two files line by line.
 *
 * @details This program compares two files line by line and prints out the lines containing differences
 * as well as the number of differing characters either to stdout or a specified file.
 * Note that lines are only compared until one of them ends. Therefore the two lines 'abc\n' and 'abcdef\n'
 * are considered to be equal by this program.
 * On the other hand, there is no limit concerning the length of the lines.
 * If no differences were found, a corresponding message will be outputted instead.
 * The program receives the paths to the two files to compare as arguments as well as the following optional arguments:
 * [-i] to specify a case-insensitive comparison
 * [-o path] to specify that the output should be written to the specified file instead of stdout.
 * If this file does not exist it will be created.
 **/
 // Because of the simplicity of the Task, it was decided, that the program didn't need to be split into multiple files.

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#define NUMBER_OF_ARGUMENTS 2


/** Convenience type to emulate a boolean value. */
typedef enum {
    false = 0,
    true = 1
} bool;

/** The program name as specified in argumentValues[0] */
static char *programName_g;


static void tryParseArguments(int argumentCounter, char *argumentValues[],
                              char **inputPath1_out, char **inputPath2_out, char **outputPath_out,
                              bool *caseInsensitive_out);

static void tryOpenFiles(const char *inputPath1, const char *inputPath2, const char *outputPath,
                         FILE **input1_out, FILE **input2_out, FILE **output_out);

static void tryPrintDifferences(FILE *input1, FILE *input2, bool caseInsensitive, FILE *output);



/**
 * @brief The entry point of the program.
 * @details This function executes the whole program.
 * It calls upon other functions to parse and verify the arguments, open the given files,
 * compare them according to the arguments and print the results to the specified output (default stdout).
 *
 * global variables used: programName_g - The program name as specified in argumentValues[0]
 *
 * @param argumentCounter The argument counter.
 * @param argumentValues  The argument values.
 *
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argumentCounter, char *argumentValues[])
{
    programName_g = argumentValues[0];

    bool caseInsensitive;
    char *inputPath1 = NULL;
    char *inputPath2 = NULL;
    char *outputPath = NULL;
    tryParseArguments(argumentCounter, argumentValues, &inputPath1, &inputPath2, &outputPath, &caseInsensitive);

    FILE *input1 = NULL;
    FILE *input2 = NULL;
    FILE *output = NULL;
    tryOpenFiles(inputPath1, inputPath2, outputPath, &input1, &input2, &output);

    tryPrintDifferences(input1, input2, caseInsensitive, output);

    fclose(input1);
    fclose(input2);
    fclose(output);
    return EXIT_SUCCESS;
}


/**
 * @brief Prints a given message as well as the program name and synopsis to stderr
 * and terminates the program with EXIT_FAILURE.
 *
 * @param message The custom message to print along with the program name and synopsis.
 */
static inline void printUsageErrorAndTerminate(const char *message)
{
    fprintf(stderr,"%s\nUSAGE: %s [-i] [-o outfile] file1 file2", message, programName_g);
    exit(EXIT_FAILURE);
}

/**
 * @brief Prints a given message as well as the program name and current content of errno to stderr
 * and terminates the program with EXIT_FAILURE.
 *
 * @param additionalMessage The custom message to print along with the program name and errno contents.
 */
static inline void printErrnoAndTerminate(const char *additionalMessage)
{
    fprintf(stderr,"%s - %s: %s", programName_g, additionalMessage, strerror(errno));
    exit(EXIT_FAILURE);
}


/**
 * @brief Parses the program arguments.
 * @details Receives the program arguments, parses them and sets the output parameters accordingly.
 * Terminates the program with EXIT_FAILURE if one of the mandatory arguments is missing or invalid
 * arguments were specified.
 *
 * @param argumentCounter The amount of arguments.
 * @param argumentValues The actual arguments.
 * @param inputPath1_out Contains a pointer to a string containing the path of the first file to compare
 * after the function terminates.
 * @param inputPath2_out Contains a pointer to a string containing the path of the second file to compare
 * after the function terminates.
 * @param outputPath_out Contains a pointer to a string containing the path of the output file
 * (or NULL for default output) after the function terminates.
 * @param caseInsensitive_out Contains a pointer to a bool enum indicating weather the comparison should
 * be done case insensitive or not after the function terminates.
 */
static void tryParseArguments(int argumentCounter, char *argumentValues[],
                              char **inputPath1_out, char **inputPath2_out, char **outputPath_out,
                              bool *caseInsensitive_out)
{
    *caseInsensitive_out = false;

    int opt;
    while((opt = getopt(argumentCounter, argumentValues, "io:")) != -1)
    {
        switch(opt)
        {
            case 'o': *outputPath_out      = optarg; break;
            case 'i': *caseInsensitive_out = true;   break;

            case '?': printUsageErrorAndTerminate("One or more invalid options");
            default:  printUsageErrorAndTerminate("Unknown option returned by getopt(3)");
        }
    }

    if ((argumentCounter - optind) != NUMBER_OF_ARGUMENTS)
        printUsageErrorAndTerminate("Wrong number of arguments");

    *inputPath1_out = argumentValues[optind];
    *inputPath2_out = argumentValues[optind + 1];
}


/**
 * @brief Tries to open the specified input files and - if configured - the output file.
 *
 * @details If a output file is specified but does not exist it will be created.
 * Terminates the program with EXIT_FAILURE if opening a file (or creating the output file if necessary) fails.

 * @param inputPath1 The path of the first file to compare.
 * @param inputPath2 The path of the second file to compare.
 * @param outputPath The path of the file to write the output into (or NULL for output over stdout).
 * @param input1_out Contains a pointer to the (in read mode) opened first input file after the function terminates.
 * @param input2_out Contains a pointer to the (in read mode) opened second input file after the function terminates.
 * @param output_out Contains a pointer to the (in write mode) opened output file (or stdout if outputPath was NULL)
 * after the function terminates.
 */
static void tryOpenFiles(const char *inputPath1, const char *inputPath2, const char *outputPath,
                         FILE **input1_out, FILE **input2_out, FILE **output_out)
{
    *input1_out = fopen(inputPath1, "r");
    *input2_out = fopen(inputPath2, "r");

    if (*input1_out == NULL || *input2_out == NULL)
        printErrnoAndTerminate("Opening input files failed");

    if (outputPath != NULL)
    {
        *output_out = fopen(outputPath, "w");
        if (*output_out == NULL)
            printErrnoAndTerminate("Opening/Creating output file failed");
    }
    else
        *output_out = stdout;
}


/**
 * @brief Checks if the given parameter is EOF or '\n'.

 * @param character The character to check.
 */
static inline bool isNewLineOrEof(int character)
{
    return character == '\n' || character == EOF;
}

/**
 * @brief Checks if any of two given parameters is EOF or '\n'.

 * @param c1 The first character to check.
 * @param c2 The second character to check.
 */
static inline bool isAnyNewLineOrEof(int c1, int c2)
{
    return isNewLineOrEof(c1) || isNewLineOrEof(c2);
}

/**
 * @brief Takes an opened File, moves the current file position to the next line
 *        and returns the first character of that line.
 *
 * @param file The already opened file in question
 *
 * @return The first character of the next line
 */
static int moveToFirstCharOfNextLine(FILE *file) {
    while (!isNewLineOrEof(fgetc(file)));
    return fgetc(file);
}

/**
 * @brief Compares the given files and prints the differing lines one by one ("No differences found!" if there are none).
 *
 * @details This function compares the two given files line by line until one of the files ends.
 * Each line is only compared until one of the lines from each file ends (so 'a\n' is equal to 'ab\n').
 * If two lines differ, the line number and the number of differences is printed immediately.
 * If the files are equal in terms of the described algorithm, "No differences found!" is printed instead.
 * In case of an error while printing out a differing line, the program terminates with EXIT_FAILURE.
 *
 * @param input1          The first file to compare.
 * @param input2          The second file to compare.
 * @param caseInsensitive Indicates weather or not the comparison should be case insensitive.
 * @param output          The file the result is to be printed to (can also be stdout).
 */
// An approach using fgetc was chosen over one using fgets to avoid having to use dynamic memory allocation
// as it is a source of many possible mistakes and would make the code arguably more complicated
static void tryPrintDifferences(FILE *input1, FILE *input2, bool caseInsensitive, FILE *output)
{
    int (*comparer)(const char*, const char*) = caseInsensitive ? &strcasecmp : &strcmp;

    int currCharFile1 = fgetc(input1);
    int currCharFile2 = fgetc(input2);

    int differingLines = 0;

    int line = 1;
    for (; currCharFile1 != EOF && currCharFile2 != EOF; line++)
    {
        int numberOfDifferencesInLine = 0;

        while (!isAnyNewLineOrEof(currCharFile1, currCharFile2) && currCharFile1 != '\r' && currCharFile2 != '\r') // \r check for Windows (NL = \r\n)
        {
            if ((*comparer)((const char *) &currCharFile1, (const char *) &currCharFile2) != 0) // C is already so 'dangerous'; I figure having no if brackets won't make the difference.
                numberOfDifferencesInLine++;

            currCharFile1 = fgetc(input1);
            currCharFile2 = fgetc(input2);
        }

        if (numberOfDifferencesInLine != 0)
        {
            differingLines++;
            if (fprintf(output, "Line: %i, differing characters: %i\n", line, numberOfDifferencesInLine) < 0)
                printErrnoAndTerminate("Printing differences failed");
        }

        currCharFile1 = moveToFirstCharOfNextLine(input1);
        currCharFile2 = moveToFirstCharOfNextLine(input2);
    }

    if (differingLines == 0)
        fprintf(output, "No differences found!");
}
