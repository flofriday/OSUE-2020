/**
 * @file   forksort.c
 * @author Tobias de Vries (e01525369)
 * @date   20.12.2020
 *
 * @brief A program to read in lines until an EOF is encountered, sort them alphabetically (case-sensitive)
 * and then output them again by recursively executing itself.
 *
 * @details This program reads lines from stdin until an EOF is encountered.
 * If only one line is read than this line is returned immediately. Otherwise the process forks twice
 * (children execute forksort again) and sends the lines to it's two children alternately.
 * Afterwards it reads the output of its children line by line, compares them, outputs the smaller one
 * and reads the next line of the respective child (if any, otherwise all output of the child still providing lines it output)
 * for comparison until all children terminated.
 * The process does not know if it outputs the result to the console or to a parent process,
 * as stdout is redirected for children.
 * The program takes no arguments. All lines are read from stdin.
 **/
/*        FDS
 *             outputPipe |
 *         W - unused end |
 *         R - access<----|---STDOUT
 * Parent                 |            Child
 *             inputPipe  |
 *         W - access-----|-->STDIN
 *         R - unused end |
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>


//region ERROR
#define TRY(result, message) try(result, message, __LINE__)
#define TRY_PTR(result, message) tryPtr(result, message, __LINE__)

//region MESSAGES
#define ERROR_REALLOC "realloc failed"

#define ERROR_LOGGING        "writing to log failed"
#define ERROR_OPEN_LOG_FILE  "Creating log failed"
#define ERROR_CREATE_LOG_DIR "Creating log directory failed"

# define ERROR_INIT_PIPE         "Initializing pipe failed"
# define ERROR_CLOSE_PIPE        "Closing pipe failed"
# define ERROR_REDIRECT_PIPE     "Redirecting pipe failed"
# define ERROR_OPEN_PIPE_ACCESS  "Opening pipe access file failed"
# define ERROR_CLOSE_PIPE_ACCESS "Closing pipe access file failed"

# define ERROR_EXEC           "Executing Process in child failed"
# define ERROR_FORK           "Forking failed"
# define ERROR_WRITE_CHILD    "Writing to child failed"
# define ERROR_WRITE_PARENT   "Writing to parent failed"
# define ERROR_CHILD_FAILURE  "Child terminated with error"
# define ERROR_WAIT_FOR_CHILD "Waiting for child completion failed"
//endregion
//endregion

//region LOGGING
#define LOGGING  false // set to true to output logs. They are written to a dedicated directory (created if it doesn't exist).
#define LOG_DIR  "Process_Logs"
#define LOG_PATH "Process_Logs/%d"

#define LOG(format, ...)                                              \
    if (LOGGING)                                                       \
        TRY(fprintf(g_process_log, format, __VA_ARGS__), ERROR_LOGGING) \
//endregion

//region TYPES
/** Convenience type to emulate a boolean value. */
typedef enum {
    true = 1,
    false = 0
} bool;

/**
 * Represents a pipe created by a call to pipe.
 * Holds the pipes file descriptors and a FILE pointer to the end of the pipe relevant for the current process
 * (eg the write end of a pipe directed from this process to a child).
 */
typedef struct {
    int fds[2];
    FILE *access;
} pipe_t;

/**
 * Represents a child process.
 * Holds its process id as well as two pipes to write to and read from the child.
 */
typedef struct {
    pid_t pid;
    pipe_t inputPipe;
    pipe_t outputPipe;
} child_process_t;

/**
 * Represents the index of the file descriptor in the int[2] array set by a call to pipe(),
 * that represents the read/write end of the pipe.
 * For example: pipe_t inputPipe; pipe(inputPipe); now write to inputPipe.fds[WRITE]
 */
typedef enum {
    READ = 0,
    WRITE = 1
} pipe_end_e;

/** Reuse pipe_end_e to also indicate the mode of a pipe. */
typedef pipe_end_e pipe_mode_e;
//endregion

//region GLOBAL VARIABLES
/** The program name as specified in argv[0] */
char *programName_g = "Not yet set";

/** The first child of the process if any. */
child_process_t child1_g;
/** The second child of the process if any. */
child_process_t child2_g;

/** A pointer to the logfile of the process if logging is enabled and was initialized. */
FILE *g_process_log;
//endregion

//region FUNCTIONS DECLARATIONS
static inline void tryOpenProcessLog(void);
static inline void tryParseArguments(int argc, char **argv);

static inline bool tryReadLineFrom(FILE* source, char **line_out, int *lineSize_out);
static inline void tryReadLineAndExitOnEOF(char **line_out);
static inline void tryForwardInputToChildren(char **currLine_in_out);
static inline void tryReadAndOutputChildrenResultsOrdered(char **lineChild1, char **lineChild2);

static inline void tryStartAndRunChild(child_process_t* childPtr);
static inline void tryOpenChildrenAccess(pipe_mode_e mode);
static inline void tryCloseChildrenAccess(pipe_mode_e mode);
static inline void tryWaitForChildCompletion(pid_t pid_child);

static inline int initPipe(pipe_t *pipePtr);
static inline int redirectPipe(pipe_t *pipePtr, pipe_end_e pipeEnd, int targetFd);
static inline int closePipeEnd(pipe_t *pipePtr, pipe_end_e pipeEnd);

static inline void try(int operationResult, const char *message, int line);
//endregion


/**
 * @brief The entry point of the program.
 * @details This function executes the whole program by calling upon other functions.
 *
 * global variables used: programName_g - The program name as specified in argumentValues[0]
 *                        child1_g      - The first child of the process if any
 *                        child2_g      - The second child of the process if any
 *                        g_process_log - A pointer to the logfile of the process if logging is enabled and was initialized.
 *
 * @param argumentCounter The argument counter.
 * @param argumentValues  The argument values.
 *
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argc, char *argv[])
{
    if (LOGGING)
        tryOpenProcessLog();
    LOG("%s", "Program started.\n\n");

    tryParseArguments(argc, argv);

    LOG("%s", "Try reading first line...\n\n");
    char *line = malloc(sizeof(char));
    tryReadLineAndExitOnEOF(&line);
    LOG("%s", "Input seems to consist of multiple lines.\n\n");

    LOG("%s", "Try initializing children...\n\n");
    tryStartAndRunChild(&child1_g);
    tryStartAndRunChild(&child2_g);
    LOG("%s", "Children up and running.\n\n");

    LOG("%s", "Try forwarding input to children...\n\n");
    tryOpenChildrenAccess(WRITE);
    tryForwardInputToChildren(&line);
    tryCloseChildrenAccess(WRITE);
    LOG("%s", "Input successfully redirected and input pipes closed.\n\n");

    LOG("%s", "Try reading output of children and output result...\n\n");
    tryOpenChildrenAccess(READ);
    char* line2 = malloc(sizeof(char));
    tryReadAndOutputChildrenResultsOrdered(&line, &line2);
    tryCloseChildrenAccess(READ);
    LOG("%s", "Result output succeeded.\n\n");

    LOG("%s", "Waiting for children to complete...\n\n");
    tryWaitForChildCompletion(child1_g.pid);
    tryWaitForChildCompletion(child2_g.pid);
    LOG("%s", "Children completed successfully.\n\n");

    free(line);
    free(line2);

    if (LOGGING)
        fclose(g_process_log);

    return EXIT_SUCCESS;
}


/**
 * @brief Verifies there are no arguments except the program name and sets it.
 * @details Terminates the program with EXIT_FAILURE if one or more arguments were specified by the user.
 *
 * global variables used: programName_g - The program name as specified in argumentValues[0]
 */
static inline void tryParseArguments(int argc, char **argv)
{
    if (argc != 1)
    {
        fprintf(stderr, "Invalid parameters. USAGE: %s\n", programName_g);
        LOG("Invalid parameters. USAGE: %s\n", programName_g);
        exit(EXIT_FAILURE);
    }

    programName_g = argv[0];
}

//region ERROR HANDLING
/**
 * @brief Prints a given message and line number as well as the program name, process id and current content of errno
 * to stderr (as well as the log file if LOGGING is true) and terminates the program with EXIT_FAILURE.
 *
 * global variables used: programName_g - The program name as specified in argumentValues[0]
 *
 * @param additionalMessage The custom message to print along with the program name and errno contents.
 * @param line              The line number to print along with the error message.
 */
static inline void printErrnoAndTerminate(const char *additionalMessage, int line)
{
    fprintf(stderr, "ERROR in %s (pid: %d, line %d) - %s, ERRNO: %s\n",
            programName_g, getpid(), line, additionalMessage, strerror(errno));

    if (LOGGING)
    {
        fprintf(g_process_log, "ERROR in %s (pid: %d, line %d) - %s, ERRNO: %s\n",
                programName_g, getpid(), line, additionalMessage, strerror(errno));

        fclose(g_process_log);
    }

    exit(EXIT_FAILURE);
}

/**
 * @brief A function to check the result of an operation and terminate if it failed.
 * @details Use by passing the result of an operation that returns a negative value upon failure as first argument,
 * along with a message and the line number that should be printed in case the result is indeed negative.
 * If the given operation result is negative the program calls printErrnoAndTerminate
 * and thereby terminates the program with EXIT_FAILURE.
 *
 * @param operationResult The result of an operation that returns a negative value upon failure.
 * @param message         The error message that should be printed in case the result is indeed negative.
 * @param line            The line that should be printed in case the result is indeed negative.
 */
static inline void try(int operationResult, const char *message, int line)
{
    if (operationResult < 0)
        printErrnoAndTerminate(message, line);
}

/** Same as try but checks for a NULL pointer instead of a negative integer. */
static inline void tryPtr(void *operationResult, const char *message, int line)
{
    if (operationResult == NULL)
        printErrnoAndTerminate(message, line);
}

/**
 * @brief Should only be called if LOGGING is true. Initializes directory and file.
 * @details Creates a Log directory specified by LOG_DIR if it doesn't exist already and opens/creates g_process_log.
 * Calls printErrnoAndTerminate on failure of any operation and thereby terminates the program with EXIT_FAILURE.
 *
 * global variables used: g_process_log - A pointer to the logfile of the process if logging is enabled and was initialized.
 */
static inline void tryOpenProcessLog(void)
{
    struct stat logDirStatus = {0};
    if (stat(LOG_DIR, &logDirStatus) == -1) {
        TRY(mkdir(LOG_DIR, 0700), ERROR_CREATE_LOG_DIR);
    }

    char logFilePath[25];
    TRY(sprintf(logFilePath, LOG_PATH, getpid()), "sprintf failed");
    TRY_PTR(g_process_log = fopen(logFilePath, "a"), ERROR_OPEN_LOG_FILE);
}
//endregion

//region INPUT HANDLING
/**
 * @brief Copies the contents of source to dest without a terminating newline if source contained one.
 * @details Terminates the program with EXIT_FAILURE if the call to strcpy fails by calling printErrnoAndTerminate.
 *
 * @param source The string to copy.
 * @param dest   The destination. Must be big enough to hold the full source.
 * @param size   The size of source incl. \0
 *
 * @return returns dest for convenience
 */
static inline char *tryCopyWithoutNewline(char *source, char *dest, int size)
{
    TRY_PTR(strcpy(dest, source), "strcpy failed");
    if (dest[size-2] == '\n')
        dest[size-2] = '\0';

    return dest;
}

/**
 * @brief Reads one line from stdin  (including \n but excluding EOF).
 * If the line is terminated by EOF instead of a newline it is printed to stdout and the program terminates with EXIT_SUCCESS.
 * @details Terminates the program with EXIT_FAILURE by calling printErrnoAndTerminate upon failure to read or print the line.
 *
 * @param line_out A pointer to a with malloc initialized memory-area. Will be resized to fit the line.
 */
static inline void tryReadLineAndExitOnEOF(char **line_out)
{
    bool terminatedByEOF = tryReadLineFrom(stdin, line_out, NULL);

    if(terminatedByEOF)
    {
        TRY(fprintf(stdout, "%s", *line_out), ERROR_WRITE_PARENT);
        LOG("%s", "Input was only a single line, process terminates.\n");

        free(*line_out);
        exit(EXIT_SUCCESS);
    }
}

/**
 * @brief Reads one line from source to line (including \n but excluding EOF).
 * Returns true if the line is terminated by EOF instead of a newline.
 * @details Terminates the program with EXIT_FAILURE by calling printErrnoAndTerminate upon failure to read the line
 * or reallocate memory.
 *
 * @param source       The FILE* the line should be read from.
 * @param line_out     A pointer to a with malloc initialized memory-area. Will be resized to fit the line.
 * @param lineSize_out Will contain the size of the line after functions completed (\0 included).
 *
 * @return true if the line was terminated by EOF, false if it was terminated by a newline.
 */
static inline bool tryReadLineFrom(FILE* source, char **line_out, int *lineSize_out)
{
    int i = 0;
    int nextChar;
    do
    {
        TRY_PTR(*line_out = realloc(*line_out, (i + 1) * sizeof(char)), ERROR_REALLOC);

        nextChar = fgetc(source);
        if (nextChar == EOF)
            break; // don't write EOF to output

        (*line_out)[i] = (char) nextChar;
        i++;
    }
    while(nextChar != '\n'); // do write \n to output

    if (nextChar == '\n')
        TRY_PTR(*line_out = realloc(*line_out, (i + 1) * sizeof(char)), ERROR_REALLOC);

    (*line_out)[i] = '\0';

    int size = i+1;
    if (lineSize_out != NULL)
        *lineSize_out = size;

    bool terminatedByEOF = i == 0 || (*line_out)[i - 1] != '\n'; // line was terminated by EOF if the char before \0 is not \n

    char lineWithoutNl[size];
    tryCopyWithoutNewline(*line_out, lineWithoutNl, size);
    LOG("Read line: %s - Terminated by EOF: %s\n", lineWithoutNl, terminatedByEOF ? "true" : "false");

    return terminatedByEOF;
}

/**
 * @brief Receives the first read line and redirects it as well as all following lines to the children of this process.
 * @details Terminates the program with EXIT_FAILURE upon failure of any called function by calling printErrnoAndTerminate.
 *
 * global variables used: child1_g - The first child of the process if any
 *                        child2_g - The second child of the process if any
 *
 * @param currLine_in_out A pointer to the first line read by the program, will be resized and therefore needs to
 * be initialized with malloc.
 */
static inline void tryForwardInputToChildren(char **currLine_in_out)
{
    child_process_t *children[2] = {&child1_g, &child2_g};

    int nextLineSize;
    char *nextLine = malloc(sizeof(char));

    bool readEOF = false;
    for (int alternatingIndex = 0; !readEOF; alternatingIndex = !alternatingIndex)
    {
        readEOF = tryReadLineFrom(stdin, &nextLine, &nextLineSize);
        if (readEOF)
        {
            *(strchr(*currLine_in_out, '\n')) = '\0'; //remove \n from currLine as it is the last line for the child (\n will always be in currLine here)
            TRY(fprintf(children[!alternatingIndex]->inputPipe.access, "%s", *currLine_in_out), ERROR_WRITE_CHILD);
            LOG("Wrote final line to child %d: %s\n", !alternatingIndex+1, *currLine_in_out);
            TRY(fprintf(children[alternatingIndex]->inputPipe.access, "%s", nextLine), ERROR_WRITE_CHILD);
            LOG("Wrote final line to child %d: %s\n", alternatingIndex+1, nextLine);
        }
        else
        {
            TRY(fprintf(children[alternatingIndex]->inputPipe.access, "%s", *currLine_in_out), ERROR_WRITE_CHILD);
            LOG("Wrote line line to child %d: %s\n", alternatingIndex+1, *currLine_in_out);

            TRY_PTR(*currLine_in_out = realloc(*currLine_in_out, nextLineSize), ERROR_REALLOC);
            strcpy(*currLine_in_out, nextLine);
        }
    }

    free(nextLine);
}

//region tryReadAndOutputChildrenResultsOrdered
/**
 * @brief Gets the access FILE* of the specified child.
 *
 * global variables used: child1_g - The first child of the process if any
 *                        child2_g - The second child of the process if any
 *
 * @param childId 1 to indicate child1_g, 2 to indicate child2_g
 *
 * @return child<childId>_g.outputPipe.access
 */
static inline FILE* getChildAccessById(int childId) {
    return childId == 1 ? child1_g.outputPipe.access : child2_g.outputPipe.access;
}

/**
 * @brief Outputs the given line to stdout and reads the next one from the specified child if there is one.
 * @details Terminates the program with EXIT_FAILURE upon failure of any called function by calling printErrnoAndTerminate.
 *
 * global variables used: child1_g - The first child of the process if any
 *                        child2_g - The second child of the process if any
 *
 * @param childId                  Indicates from which child the next line should be read; 1 for child1_g, 2 for child2_g
 * @param line_in_out              A pointer to the line that should be printed, will be resized and overwritten if a new line is read.
 * @param lineSize_in_out          The size of the given line. Will be overwritten if a new line is read.
 * @param isLastLineOfChild_in_out Indicates if the given line is the last line of the child. Will be overwritten if a new line is read.
 * @param withNewline              Indicates weather the line should be printed with a trailing newline.
 *
 * @return true if the last line of the child was output, false otherwise.
 */
static inline bool tryOutputAndReadNextIfNotLast(int childId, char **line_in_out, int *lineSize_in_out,
                                                 bool *isLastLineOfChild_in_out, bool withNewline)
{
    char lineWithoutNl[*lineSize_in_out];
    tryCopyWithoutNewline(*line_in_out, lineWithoutNl, *lineSize_in_out);

    LOG("Output line of child %d: %s - %s.\n", childId, lineWithoutNl, withNewline ? "NL" : "EOF");
    TRY(fprintf(stdout, "%s%c", lineWithoutNl, withNewline ? '\n' : '\0'), ERROR_WRITE_PARENT);
    if (*isLastLineOfChild_in_out)
    {
        LOG("Completed output of child %d.\n\n", childId);
        return true;
    }

    *isLastLineOfChild_in_out = tryReadLineFrom(getChildAccessById(childId), line_in_out, lineSize_in_out);
    return false;
}

/**
 * @brief Outputs lines read from the specified child until the child's output completed.
 * @details Terminates the program with EXIT_FAILURE upon failure of any called function by calling printErrnoAndTerminate.

 * @param childId         1 to indicate child1_g, 2 to indicate child2_g
 * @param line            A pointer to the current line read from the child. Needs to be malloc initialized, will be reused.
 * @param terminatedByEOF Indicates if the current line was terminated by EOF
 * @param lineSize        The size of the current line. Will be reused.
 */
static inline void tryOutputRemainingLinesOfChild(int childId, char **line, bool terminatedByEOF, int *lineSize)
{
    LOG("Output remaining lines of child %d.\n", childId);

    bool completed = false;
    while (!completed) {
        completed = tryOutputAndReadNextIfNotLast(childId, line, lineSize, &terminatedByEOF, !terminatedByEOF);
    }
}
//endregion
/**
 * @brief Processes and outputs the output of both children line by line.
 * @details Reads the output of both children line by line, compares them, outputs the smaller one (alphabetically)
 * and reads the next line of the respective child (if any, otherwise all output of the child still providing lines it output)
 * for comparison until the output of both children was processed.
 * Terminates the program with EXIT_FAILURE upon failure of any called function by calling printErrnoAndTerminate.

 * @param lineChild1 A pointer to the current line read from child1_g. Needs to be malloc initialized, will be reused.
 * @param lineChild2 A pointer to the current line read from child2_g. Needs to be malloc initialized, will be reused.
 */
static inline void tryReadAndOutputChildrenResultsOrdered(char **lineChild1, char **lineChild2)
{
    int c1lineSize;
    int c2lineSize;
    bool c1completed = false;
    bool c2completed = false;

    bool c1readEOF = tryReadLineFrom(child1_g.outputPipe.access, lineChild1, &c1lineSize);
    bool c2readEOF = tryReadLineFrom(child2_g.outputPipe.access, lineChild2, &c2lineSize);

    do
    {
        char line1withoutNl[c1lineSize];
        char line2withoutNl[c2lineSize];
        tryCopyWithoutNewline(*lineChild1, line1withoutNl, c1lineSize);
        tryCopyWithoutNewline(*lineChild2, line2withoutNl, c2lineSize);

        LOG("\nCompare lines (c1: %s | c2: %s).\n", line1withoutNl, line2withoutNl);

        if (strcmp(*lineChild1, *lineChild2) < 0)
            c1completed = tryOutputAndReadNextIfNotLast(1, lineChild1, &c1lineSize, &c1readEOF, true);
        else
            c2completed = tryOutputAndReadNextIfNotLast(2, lineChild2, &c2lineSize, &c2readEOF, true);
    }
    while(!(c1completed || c2completed));

    if (c1completed)
        tryOutputRemainingLinesOfChild(2, lineChild2, c2readEOF, &c2lineSize);
    else
        tryOutputRemainingLinesOfChild(1, lineChild1, c1readEOF, &c1lineSize);
}
//endregion

//region PIPE UTILITY
/**
 * @brief Calls pipe() on the file descriptor array of the given pipe_t.
 *
 * @param pipePtr A pointer to the pipe that should be initialized.
 * @return The exit code of pipe().
 */
static inline int initPipe(pipe_t *pipePtr) {
    return pipe(pipePtr->fds);
}

/**
 * @brief Calls fdopen() on the file descriptor of the given pipe indicated by mode and sets the pipes access to the result.
 *
 * @param pipe The pipe to open the access for.
 * @param mode The mode to open the pipe in.
 * @return The result of the call to fdopen().
 */
static inline void *openPipeAccess(pipe_t *pipe, pipe_mode_e mode) {
    return pipe->access = fdopen(pipe->fds[mode], mode == WRITE ? "w" : "r");
}

/**
 * @brief Calls close() on the file descriptor of the given pipe indicated by pipeEnd.
 *
 * @param pipePtr A pointer to the pipe whose file descriptor should be closed.
 * @return The exit code of close().
 */
static inline int closePipeEnd(pipe_t *pipePtr, pipe_end_e pipeEnd) {
    return close(pipePtr->fds[pipeEnd]);
}

/**
 * @brief Calls fclose() on the access of the given pipe.
 *
 * @param pipePtr A pointer to the pipe whose access should be closed.
 * @return The exit code of fclose().
 */
static inline int closePipeAccess(pipe_t *pipePtr) {
    return fclose(pipePtr->access); // Also closes underlying file descriptor
}

/**
 * @brief Redirects the file descriptor of the given pipe indicated by pipeEnd to the given target using dup2().
 * Afterwards calls closePipeEnd() for that file descriptor.
 *
 * @param pipePtr A pointer to the pipe that should be redirected.
 * @return -1 if dup2() or closePipeEnd() fails, 0 otherwise.
 */
static inline int redirectPipe(pipe_t *pipePtr, pipe_end_e pipeEnd, int targetFd)
{
    if (dup2(pipePtr->fds[pipeEnd], targetFd) == -1)
        return -1;

    if (closePipeEnd(pipePtr, pipeEnd) == -1)
        return -1;

    return 0;
}
//endregion

//region CHILDREN
/**
 * @brief Initializes and executes the child pointed to by childPtr.
 * @details Initializes input and output pipe of the child, forks and calls execlp for the child.
 * Also handles closing of unused pipe-ends in child & parent.
 * Terminates the program with EXIT_FAILURE upon failure of any called function by calling printErrnoAndTerminate.
 *
 * @param childPtr A pointer to the child that should be initialized and executed.
 */
static inline void tryStartAndRunChild(child_process_t* childPtr)
{
    static int childNumber = 0;

    TRY(initPipe(&(childPtr->inputPipe)), ERROR_INIT_PIPE);
    TRY(initPipe(&(childPtr->outputPipe)), ERROR_INIT_PIPE);
    LOG("Initialized input & output pipe for child %d.\n", ++childNumber);

    TRY(childPtr->pid = fork(), ERROR_FORK);
    if (childPtr->pid == 0) // child continues here
    {
        TRY(closePipeEnd(&(childPtr->inputPipe), WRITE), ERROR_CLOSE_PIPE);
        TRY(redirectPipe(&(childPtr->inputPipe), READ, STDIN_FILENO), ERROR_REDIRECT_PIPE);

        TRY(closePipeEnd(&(childPtr->outputPipe), READ), ERROR_CLOSE_PIPE);
        TRY(redirectPipe(&(childPtr->outputPipe), WRITE, STDOUT_FILENO), ERROR_REDIRECT_PIPE);

        if (childPtr == &child2_g)
        {
            TRY(closePipeEnd(&child1_g.inputPipe, WRITE), ERROR_CLOSE_PIPE);
            TRY(closePipeEnd(&child1_g.outputPipe, READ), ERROR_CLOSE_PIPE);
        }

        TRY(execlp(programName_g, programName_g, NULL), ERROR_EXEC);
        // shouldn't be reached
    }

    TRY(closePipeEnd(&(childPtr->inputPipe), READ), ERROR_CLOSE_PIPE);
    TRY(closePipeEnd(&(childPtr->outputPipe), WRITE), ERROR_CLOSE_PIPE);
    LOG("Closed unused pipe ends of child %d.\n", childPtr->pid);
}

/**
 * @brief Waits for the child identified by its process id to complete.
 * Terminates the program with EXIT_FAILURE by calling printErrnoAndTerminate if the child didn't terminate with EXIT_SUCCESS.
 *
 * @param pid_child The process id of the child to wait for.
 */
static inline void tryWaitForChildCompletion(pid_t pid_child)
{
    int status;
    while (waitpid(pid_child, &status, 0) == -1) {
        if (errno == EINTR)
            continue;

        printErrnoAndTerminate(ERROR_WAIT_FOR_CHILD, __LINE__);
    }

    if (WEXITSTATUS(status) != EXIT_SUCCESS) // NOLINT(hicpp-signed-bitwise)
        printErrnoAndTerminate(ERROR_CHILD_FAILURE, __LINE__);

    LOG("Child %d terminated successfully.\n", pid_child);
}

/**
 * @brief Calls openPipeAccess() for the input or output pipe of each child depending on the given mode.
 * Terminates the program with EXIT_FAILURE by calling printErrnoAndTerminate if openPipeAccess() fails.
 * @details global variables used: child1_g - The first child of the process if any
 *                                 child2_g - The second child of the process if any
 *
 * @param mode WRITE to open input-pipes, READ to open output-pipes
 */
static inline void tryOpenChildrenAccess(pipe_mode_e mode)
{
    if (mode == WRITE)
    {
        TRY_PTR(openPipeAccess(&child1_g.inputPipe, mode), ERROR_OPEN_PIPE_ACCESS);
        TRY_PTR(openPipeAccess(&child2_g.inputPipe, mode), ERROR_OPEN_PIPE_ACCESS);
        LOG("Successfully opened input-pipe access of children (%d, %d).\n", child1_g.pid, child2_g.pid);
    }
    else
    { //mode == READ
        TRY_PTR(openPipeAccess(&child1_g.outputPipe, mode), ERROR_OPEN_PIPE_ACCESS);
        TRY_PTR(openPipeAccess(&child2_g.outputPipe, mode), ERROR_OPEN_PIPE_ACCESS);
        LOG("Successfully opened output-pipe access of children (%d, %d).\n", child1_g.pid, child2_g.pid);
    }
}

/**
 * @brief Calls closePipeAccess() for the input or output pipe of each child depending on the given mode.
 * Terminates the program with EXIT_FAILURE by calling printErrnoAndTerminate if closePipeAccess() fails.
 * @details global variables used: child1_g - The first child of the process if any
 *                                 child2_g - The second child of the process if any
 *
 * @param mode WRITE to close input-pipes, READ to close output-pipes
 */
static inline void tryCloseChildrenAccess(pipe_mode_e mode)
{
    if (mode == WRITE) {
        TRY(closePipeAccess(&child1_g.inputPipe), ERROR_CLOSE_PIPE_ACCESS);
        TRY(closePipeAccess(&child2_g.inputPipe), ERROR_CLOSE_PIPE_ACCESS);
        LOG("Successfully closed input-pipe access of children (%d, %d).\n", child1_g.pid, child2_g.pid);
    }
    else { //mode == READ
        TRY(closePipeAccess(&child1_g.outputPipe), ERROR_CLOSE_PIPE_ACCESS);
        TRY(closePipeAccess(&child2_g.outputPipe), ERROR_CLOSE_PIPE_ACCESS);
        LOG("Successfully closed output-pipe access of children (%d, %d).\n", child1_g.pid, child2_g.pid);
    }
}
//endregion
