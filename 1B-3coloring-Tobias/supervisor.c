/**
 * @file   main.c
 * @author Tobias de Vries (e01525369)
 * @date   20.11.2020
 *
 * @brief Contains the main functionality of the 3color - Supervisor program.
 *
 * @details The Supervisor initializes shared memory and semaphores for its generators,
 *          determines which of the generated solutions is the best so far and repeatedly writes new found best solutions
 *          to stdout. The program shuts down when receiving SIGINT, SIGTERM or a solution with 0 edges,
 *          meaning the graph given to the generators is 3-colorable.
 *          It also manages when the generators should shut down and frees the shared memory and all semaphores afterwards.
 **/

#include "3color.h"

#define INPUT_ARGUMENT_NUMBER_ERROR "This program does not support any arguments."

typedef struct sigaction sigaction_t;

/** The program name as specified in argumentValues[0]. This variable is declared in util.h */
char *programName_g;

/** A global flag indicating whether the application should shut down or not. */
volatile sig_atomic_t shouldTerminate_g = 0;

static inline void initializeSignalHandler(int signal, void (*handler)(int), sigaction_t *sa_out);
static inline int8_t overwriteAndPrintIfBetter(solution_t newSolution, solution_t *currentBestSolution);


/**
 * @brief A Signalhandler for SIGINT & SIGTERM Signals. Initiates shutdown of application.
 * @details uses the global variable shouldTerminate_g to do so.
 *
 * @param _ required by sigaction.sa_handler.
 */
void initiateTermination(int _) {
    shouldTerminate_g = true;
}



/**
 * @brief The entry point of the supervisor program.
 * @details This function executes the whole program.
 *          It calls upon other functions to verify the program arguments,
 *          create and initialize the shared memory and all semaphores and keeps track of the best solutions.
 *
 * global variables used: programName_g - The program name as declared in util.h and specified in argumentValues[0]
 *
 * @param argumentCounter The argument counter. Must be 1 as no arguments are allowed.
 * @param argumentValues  The argument values. Must only contain the program name.
 *
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
 int main(int argumentCounter, char *argumentValues[])
{
    //region INIT
    programName_g = argumentValues[0];

    if (argumentCounter > 1)
        printErrorAndTerminate(INPUT_ARGUMENT_NUMBER_ERROR);

    sigaction_t sigInt;
    sigaction_t sigTerm;
    initializeSignalHandler(SIGINT, initiateTermination, &sigInt);
    initializeSignalHandler(SIGTERM, initiateTermination, &sigTerm);

    int shared_memory_fd;
    shared_memory_t *solutionBuffer;
    initializeSharedMemoryAsServer(&shared_memory_fd, &solutionBuffer);

    sem_t *freeSpaceSemaphore;
    sem_t *usedSpaceSemaphore;
    sem_t *exclWriteSemaphore;
    initializeSemaphoreAsServer(&freeSpaceSemaphore, FREE_SPACE_SEM_NAME, FREE_SPACE_SEM_INIT);
    initializeSemaphoreAsServer(&usedSpaceSemaphore, USED_SPACE_SEM_NAME, USED_SPACE_SEM_INIT);
    initializeSemaphoreAsServer(&exclWriteSemaphore, EXCL_WRITE_SEM_NAME, EXCL_WRITE_SEM_INIT);
    //endregion

    solution_t currentBestSolution = { .numberOfEdges = MAX_SOLUTION_SIZE + 1 };

    unsigned long currentReadPosition = 0; // prepare for SHM sizes as big as possible
    while (!shouldTerminate_g)
    {
        if (sem_wait(usedSpaceSemaphore) == -1) { // wait if nothing new was written yet, then read the next solution
            if (errno == EINTR) continue;
            else printErrnoAndTerminate(USED_SPACE_SEM_NAME);
        }

        if (overwriteAndPrintIfBetter(solutionBuffer->solutions[currentReadPosition], &currentBestSolution) == 0)
        {
            fprintf(stdout, FOUND_0_EDGE_SOLUTION_MSG);
            break;
        }

        if (++currentReadPosition == MAX_SOLUTION_SIZE)
            currentReadPosition = 0;

        exitOnFailure(sem_post(freeSpaceSemaphore), FREE_SPACE_SEM_NAME); // indicate one solution was read
    }
    fprintf(stdout, "\n"); // ensure newline in shell after Program stop

    //region CLEANUP
    solutionBuffer->shutdownRequested = true;

    cleanupSharedMemoryAsServer(shared_memory_fd, solutionBuffer);

    cleanupSemaphoreAsServer(freeSpaceSemaphore, FREE_SPACE_SEM_NAME);
    cleanupSemaphoreAsServer(usedSpaceSemaphore, USED_SPACE_SEM_NAME);
    cleanupSemaphoreAsServer(exclWriteSemaphore, EXCL_WRITE_SEM_NAME);
    //endregion

    return EXIT_SUCCESS;
}


/**
 * @brief Initializes a sigaction to act as specified by handler when the program receives the specified signal.
 *
 * @param signal  The signal that should be handled in a certain way.
 * @param handler The handler that should be called upon receiving the specified signal.
 * @param sa_out  The pointer to the sigaction that should be initialized.
 */
static inline void initializeSignalHandler(int signal, void (*handler)(int), sigaction_t *sa_out)
{
    memset(sa_out, 0, sizeof(*sa_out));
    (*sa_out).sa_handler = handler;
    sigaction(signal, sa_out, NULL);
}

/**
 * @brief Prints an edge_t in a given format to stdout.
 * @details Terminates the program with EXIT_FAILURE upon failure of fprintf.
 *
 * @param format The format to print the edge in. Must contain "%lu-%lu" and no other placeholders.
 * @param edge   The edge to print.
 */
static inline void printEdge(const char *format, edge_t edge) {
    exitOnFailure(fprintf(stdout, format, edge.firstNodeIndex, edge.secondNodeIndex),"Unexpected Error in fprintf");
}

/**
 * @brief Compares two solutions against each other. If newSolution is better it is printed to stdout and
 *        the content of currentBestSolution is overwritten.
 * @details The quality is determined by the number of edges in the solution. If newSolution has less edges than
 *          currentBestSolution it is considered better. If this is the case its edges are printed using printEdge.
 *          Terminates the program with EXIT_FAILURE upon failure of a called function and prints the corresponding error.
 *
 * @param newSolution         A new solution transmitted to the supervisor by a generator.
 * @param currentBestSolution A pointer to the best solution transmitted to the supervisor so far.
 *
 * @return the number of edges in newSolution if it is better than the currently best solution, -1 otherwise
 */
static inline int8_t overwriteAndPrintIfBetter(solution_t newSolution, solution_t *currentBestSolution)
{
    if (newSolution.numberOfEdges >= currentBestSolution->numberOfEdges)
        return -1;

    if (newSolution.numberOfEdges == 0)
        return 0;

    memcpy(currentBestSolution->edges, newSolution.edges, SOLUTION_EDGE_ARRAY_SIZE);
    currentBestSolution->numberOfEdges = newSolution.numberOfEdges;

    printEdge("Best Solution so far: %lu-%lu", newSolution.edges[0]);
    for (int i = 1; i < newSolution.numberOfEdges; i++)
        printEdge(", %lu-%lu", newSolution.edges[i]);

    exitOnFailure(fprintf(stdout, "\n"), "Unexpected Error in fprintf");

    return newSolution.numberOfEdges;
}
