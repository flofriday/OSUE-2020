#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>  // weil sonst sagt er immer ein warning das errno declaration not provided
#include <strings.h> // weil sonst 'warning' [implicit declaration of function 'strcasecmp' is invalid in C99]
#include <math.h>
#include <sys/errno.h>

#define debug(fmt, ...)                        \
    (void)fprintf(stderr, "[%s:%d] " fmt "\n", \
                  __FILE__, __LINE__,          \
                  ##__VA_ARGS__)

// this has to be global otherwise it cannot be used in usage();
char *programname;

/**
 * @brief the usage of the programm
 * 
 * Prints the usage of the programm in case the input isnt correct
 */
void usage(void)
{
    fprintf(stderr, "Usage: %s [-i] [-o outfile] file1 file2\n", programname);
    exit(EXIT_FAILURE);
}

/**
 * @brief closes the file*
 * 
 * Properly closes a file in case of error there is an message
 * 
 * @param stream the stream to close
 */
void properclose(FILE *stream)
{
    if (fclose(stream) == EOF)
    {
        fprintf(stderr, "%s: fclose failed: %s\n", programname, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Get the Filenames From Option object
 * 
 * Get the two filenames from the options
 * Returns an array with two C-Strings
 * 
 * @param argc the arugment counter
 * @param argv  the arguments
 * @return char**  the filenames from the options
 */
char **getFilenamesFromOption(int argc, char *argv[], char **filenames)
{
    // check if filenames are present
    if ((argc - optind) != 2)
    {
        fprintf(stderr, "%s: You used the program wrong!\n", programname);
        usage();
        exit(EXIT_FAILURE);
    }
    filenames[0] = argv[optind];
    filenames[1] = argv[optind + 1];

    return filenames;
}

/**
 * @brief read a file
 * 
 * Reads a file and handles errors on occure
 * 
 * @param filename the filename from wheer to read
 * @return FILE*  the filepointer to the given filename
 */
FILE *readFile(const char *filename)
{
    //debug("Reading file %s", filename);
    FILE *returnFile = fopen(filename, "r");

    if (returnFile == NULL)
    {
        fprintf(stderr, "%s: fopen failed: %s\n", programname, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return returnFile;
}

/**
 * @brief opens a file in append mode
 * 
 * Opens a file and handles errors on occurs
 * 
 * @param filename the filename path 
 * @return FILE* the filepointer to the given filename
 */
FILE *openFileAppend(const char *filename)
{
    //debug("Reading file %s", filename);
    FILE *returnFile = fopen(filename, "a");

    if (returnFile == NULL)
    {
        fprintf(stderr, "%s: fopen failed: %s\n", programname, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return returnFile;
}

/**
 * @brief Create a Output File object
 * 
 * Creates a file or overrides with empty string and handles errors on occurs
 * 
 * @param filename the name of the outputfile
 */
void createOutputFile(const char *filename)
{
    //debug("Reading file %s", filename);
    FILE *returnFile = fopen(filename, "w");

    if (returnFile == NULL)
    {
        fprintf(stderr, "%s: fopen failed: %s\n", programname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fputs("", returnFile) == EOF)
    {
        fprintf(stderr, "%s: fputs failed: %s\n", programname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fflush(returnFile) == EOF)
    {
        fprintf(stderr, "%s: fflush failed: %s\n", programname, strerror(errno));
        exit(EXIT_FAILURE);
    }
    properclose(returnFile);
}

/**
 * @brief compares two lines
 * 
 * Compares two lines based on caseSensitive or not and tells the amount of differences
 * 
 * @param line1 cstring line1
 * @param sizeLine1  the size of line1
 * @param line2  cstring line2
 * @param sizeLine2  the size of line2
 * @param caseSensitive  whether its case sensitive or not
 * @return int the amount of differences
 */
int compareToLines(char *line1, int sizeLine1, char *line2, int sizeLine2, int caseSensitive)
{
    if (sizeLine1 < 0 || sizeLine2 < 0)
    {
        fprintf(stderr, "%s: Error with line comparasing, because one of the lines had a negativ size", programname);
        exit(EXIT_FAILURE);
    }

    //int sizeBound = fmin(sizeLine1, sizeLine2);
    int sizeBound = -1;
    if (sizeLine1 > sizeLine2)
    {
        sizeBound = sizeLine2;
    }
    else
    {
        sizeBound = sizeLine1;
    }

    int differentCounter = 0;

    for (int i = 0; i < sizeBound - 1; i++)
    {
        char currentLine1 = line1[i];
        char currentLine2 = line2[i];

        //debug("%c \t %c",currentLine1,currentLine2);

        // converting to a string
        char c1[2];
        c1[0] = currentLine1;
        c1[1] = 0;

        char c2[2];
        c2[0] = currentLine2;
        c2[1] = 0;

        //debug("%s \t %s",c1,c2);
        //debug("Case: %i", caseSensitive);

        int compare = (caseSensitive == 1) ? strcmp(c1, c2) : strcasecmp(c1, c2);
        if (compare != 0)
        {
            differentCounter++;
        }
    }
    return differentCounter;
}

/**
 * @brief output of the diff
 * 
 * Outputs to the std and in case of a given outputfile to the file
 * 
 * @param lineCounter the current line number
 * @param linedifferences the number of diffs
 * @param outputFilename the filename of the output file
 */
void output(int lineCounter, int linedifferences, char *outputFilename)
{
    // for sprintf i need to alloc memory
    // 20 character plus the size of a integer
    char *outputString = malloc(20 + sizeof(int) * 2);
    sprintf(outputString, "Line: %i, characters: %i\n", lineCounter, linedifferences);
    if (outputFilename == NULL)
    {
        printf("%s", outputString);
    }
    else
    {
        // write to file
        FILE *outputFile = openFileAppend(outputFilename);

        if (fputs(outputString, outputFile) == EOF)
        {
            fprintf(stderr, "%s: fputs failed: %s\n", programname, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fflush(outputFile) == EOF)
        {
            fprintf(stderr, "%s: fflush failed: %s\n", programname, strerror(errno));
            exit(EXIT_FAILURE);
        }
        properclose(outputFile);
    }

    free(outputString);
}

/**
 * @brief the main
 * 
 * @param argc the argument counter 
 * @param argv the arguments
 * @return int exit code
 */
int main(int argc, char *argv[])
{
    // getting the programname for outputs
    programname = argv[0];

    // optionreading
    int caseSensitive = 1; // 1 - it is case sensitive (base case)
                           //  0 - its not case sensitive
    char *outputFilename = NULL;
    int c = -1;
    while ((c = getopt(argc, argv, "io:")) != -1)
    {
        switch (c)
        {
        case 'i':
            caseSensitive = 0; // dont mind case
            break;
        case 'o':
            outputFilename = optarg;
            createOutputFile(outputFilename);
            break;
        case '?': /* invalid option */
            usage();
            break;
        default:
            // no case found -> its an error
            usage();
            break;
        }
    }

    //debug("Casesensitive:   %i", caseSensitive);
    char *returnArray[2];
    char **filenames = getFilenamesFromOption(argc, argv, returnArray);
    if (filenames == NULL)
    {
        fprintf(stderr, "%s: Couldnt read diff files\n", programname);
        exit(EXIT_FAILURE);
    }
    const char *filename1 = filenames[0];
    const char *filename2 = filenames[1];

    //debug("%s", filename1);
    //debug("%s", filename2);

    FILE *file1 = readFile(filename1);
    FILE *file2 = readFile(filename2);

    // let getline automatically decide the size of the buffer
    char *lineFile1 = NULL;
    size_t lenFile1 = 0; // length
    ssize_t readFile1;   // size of the line im reading
    int lineCounter = 1;
    while ((readFile1 = getline(&lineFile1, &lenFile1, file1)) != -1)
    {

        char *lineFile2 = NULL;
        size_t lenFile2 = 0;
        ssize_t readFile2;
        if ((readFile2 = getline(&lineFile2, &lenFile2, file2)) != -1)
        {
            //debug("%s", lineFile2); // content file1

            int linedifferences = compareToLines(lineFile1, readFile1, lineFile2, readFile2, caseSensitive);
            if (linedifferences != 0)
            {
                output(lineCounter, linedifferences, outputFilename);
            }
        }
        else
        {
            // there is no more line to check because file2 has no space anymore
            break;
        }

        if (lineFile2)
        {
            free(lineFile2);
        }
        readFile1 = 0;
        readFile2 = 0;
        lineCounter++;
    }

    // closing everything
    properclose(file1);
    properclose(file2);

    // memory cleaning
    if (lineFile1)
    {
        free(lineFile1);
    }

    return EXIT_SUCCESS;
}
