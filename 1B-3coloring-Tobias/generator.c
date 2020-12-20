/**
 * @file   main.c
 * @author Tobias de Vries (e01525369)
 * @date   20.11.2020
 *
 * @brief Contains the main functionality of the 3color - Generator program.
 *
 * @details The Generator accepts a number of edges defining a graph, opens the shared memory and semaphores
 *          initialized by a supervisor, generates new solutions for the given graph repeatedly and
 *          writes every new solution to the shared memory.
 *          The program shuts down when the supervisors sets the corresponding flag in the shared memory.
 **/
// It was chosen to make the generator only accept 2 edges or more, since that guarantees that there are at least 3 nodes in the graph.
// It was also deliberately chosen to let the generator accept graphs such as 0-1 2-3, where not all nodes are connected,
// as this provides the user with more options while not making the program more complicated or prone to mistakes.
// Ideally the program should print a warning in this case. However, for the purposes of this exercise this was waived.

#include <inttypes.h>
#include <time.h>
#include "3color.h"


#define NUMBER_OF_NECESSARY_EDGES 2

#define INPUT_ARGUMENT_NUMBER_ERROR "A valid input graph needs at least 3 nodes. SYNOPSIS: generator EDGE1 ... EDGEn (EXAMPLE: generator 0-1 1-2)"
#define INPUT_ARGUMENT_FORMAT_ERROR "Valid edges (format %lu-%lu) are the only allowed parameters. SYNOPSIS: generator EDGE1 ... EDGEn (EXAMPLE: generator 0-1 1-2)"

/** The program name as specified in argumentValues[0]. This variable is declared in util.h */
char *programName_g;


static inline void tryParseArguments(int argumentCounter, char *argumentValues[],
                                     edge_t *edges_out[], int *numberOfEdges_out,
                                     node_t *nodes_out[], int *numberOfNodes_out);

static inline int generateSolution(node_t *nodes, int numberOfNodes, edge_t *edges, int numberOfEdges,
                                   solution_t *solution_out);

static inline void writeSolutionToShm(shared_memory_t *shared_memory, solution_t *solution);


/**
 * @brief The entry point of the generator program.
 * @details This function executes the whole program.
 *          It calls upon other functions to verify and parse the program arguments,
 *          opens the shared memory and all semaphores initialized by a supervisor and generates new solutions.
 *
 * global variables used: programName_g - The program name as declared in util.h and specified in argumentValues[0]
 *
 * @param argumentCounter The argument counter.
 * @param argumentValues  The argument values. Must contain only valid edges (format %ul-%ul) and at least 2 of them.
 *
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argumentCounter, char *argumentValues[]) {

    //region INIT
    programName_g = argumentValues[0];

    node_t *nodes;
    int numberOfNodes;
    edge_t *edges;
    int numberOfEdges;
    tryParseArguments(argumentCounter, argumentValues, &edges, &numberOfEdges, &nodes, &numberOfNodes);

    int shared_memory_fd;
    shared_memory_t *solutionBuffer;
    initializeSharedMemoryAsClient(&shared_memory_fd, &solutionBuffer);

    sem_t *freeSpaceSemaphore;
    sem_t *usedSpaceSemaphore;
    sem_t *exclWriteSemaphore;
    initializeSemaphoreAsClient(&freeSpaceSemaphore, FREE_SPACE_SEM_NAME);
    initializeSemaphoreAsClient(&usedSpaceSemaphore, USED_SPACE_SEM_NAME);
    initializeSemaphoreAsClient(&exclWriteSemaphore, EXCL_WRITE_SEM_NAME);

    srand(getpid()); // Initialize random number generator
    // endregion

    while(!solutionBuffer->shutdownRequested)
    {
        solution_t solution = { .numberOfEdges = 0 };
        if (generateSolution(nodes, numberOfNodes, edges, numberOfEdges, &solution) != -1)
        {
            if (sem_wait(exclWriteSemaphore) == -1) { // Ensure only this generator edits the shm
                if (errno == EINTR) continue; // This discards the currently generated solution but since it is not expected to happen frequently that is accepted
                else printErrnoAndTerminate(EXCL_WRITE_SEM_NAME);
            }

            if (sem_wait(freeSpaceSemaphore) == -1) { // Ensure there is free space
                if (errno == EINTR) continue; // This discards the currently generated solution but since it is not expected to happen frequently that is accepted
                else printErrnoAndTerminate(FREE_SPACE_SEM_NAME);
            }

            writeSolutionToShm(solutionBuffer, &solution);

            exitOnFailure(sem_post(usedSpaceSemaphore), USED_SPACE_SEM_NAME); // Indicate that a solution was written to the shm
            exitOnFailure(sem_post(exclWriteSemaphore), EXCL_WRITE_SEM_NAME); // Allow other generators to edit the shm again
        }
    }

    //region CLEANUP
    cleanupSharedMemoryAsClient(shared_memory_fd, solutionBuffer);

    cleanupSemaphoreAsClient(freeSpaceSemaphore);
    cleanupSemaphoreAsClient(usedSpaceSemaphore);
    cleanupSemaphoreAsClient(exclWriteSemaphore);

    free(nodes);
    free(edges);
    //endregion

    return EXIT_SUCCESS;
}


/**
 * @brief Writes the given solution to the given shared memory and modifies its currentWriteIndex accordingly.
 *
 * @param shared_memory A pointer to the shared memory the solution is to be written to.
 * @param solution      A pointer to the solution that is to be written.
 */
static inline void writeSolutionToShm(shared_memory_t *shared_memory, solution_t *solution)
{
    memcpy((shared_memory->solutions)[shared_memory->currentWriteIndex].edges, solution->edges, SOLUTION_EDGE_ARRAY_SIZE);
    (shared_memory->solutions)[shared_memory->currentWriteIndex].numberOfEdges = solution->numberOfEdges;

    if (++(shared_memory->currentWriteIndex) == MAX_SOLUTION_SIZE)
        shared_memory->currentWriteIndex = 0;
}

/**
 * @brief Assigns a random 'color' (0,1 or 2) to each node in a given array of nodes.
 *
 * @param nodes         A pointer to the nodes that should have a new color assigned to them.
 * @param numberOfNodes The number of nodes stored at the given address.
 */
static inline void colorNodesRandomly(node_t *nodes, int numberOfNodes)
{
    for (int i = 0; i < numberOfNodes; i++)
        nodes[i].color = (rand() % 3); // NOLINT(cert-msc30-c, cert-msc50-cpp)
}

/**
 * @brief Searches for the first node with the specified index in the given array of nodes
 *        and returns a pointer to it or NULL if no such node was found.
 *
 * @param nodes         A pointer to the first element of the node array.
 * @param numberOfNodes The number of nodes in the array.
 * @param nodeIndex     The searched index.
 *
 * @return A pointer to the found node or NULL if no such node was found.
 */
static inline node_t* getNodePtr(node_t *nodes, int numberOfNodes, unsigned long nodeIndex)
{
    for (int i = 0; i < numberOfNodes; i++) {
        if (nodes[i].nodeIndex == nodeIndex)
            return &nodes[i];
    }

    return NULL;
}

/**
 * @brief This function accepts a graph in the form of a number of nodes and edges and generates
 *        a set of edges (a solution_t) that can be removed to make it 3-colorable.
 * @details Solutions with more than 3color.h/MAX_SOLUTION_SIZE edges are waived and -1 is returned to indicate this.
 *
 * @param nodes         A pointer to the array of nodes.
 * @param numberOfNodes The number of nodes stored at the given node array.
 * @param edges         A pointer to the array of edges.
 * @param numberOfEdges The number of edges stored at the given edge array.
 * @param solution_out  A pointer to a solution struct in which the solution is to be stored.
 *
 * @return -1 if the solution was too big and got waived, otherwise the number of edges in the solution.
 */
static inline int generateSolution(node_t *nodes, int numberOfNodes, edge_t *edges, int numberOfEdges,
                                   solution_t *solution_out)
{
    solution_out->numberOfEdges = 0;
    colorNodesRandomly(nodes, numberOfNodes);

    int i = 0;
    for (int s = 0; i < numberOfEdges; i++)
    {
        int8_t firstNodeColor  = getNodePtr(nodes, numberOfNodes, edges[i].firstNodeIndex)->color;
        int8_t secondNodeColor = getNodePtr(nodes, numberOfNodes, edges[i].secondNodeIndex)->color;
        if (firstNodeColor == secondNodeColor)
        {
            if (solution_out->numberOfEdges == MAX_SOLUTION_SIZE) // Already found more edges that need to be removed than the allowed amount
                break;

            memcpy(&(solution_out->edges[s]), &(edges[i]), sizeof(edge_t));
            solution_out->numberOfEdges++;
            s++;
        }
    }

    return i == numberOfEdges ? 0 : -1; // Indicate whether the solution is valid or if it was too big and got canceled
}

/**
 * @brief Tries to convert a string to an edge_t. For this to succeed the string must have the format "%ul-%ul".
 *
 * @param possibleEdge The string that should be converted to an edge if possible.
 * @param edge_out     A pointer to the generated edge.
 *
 * @return -1 on failure and 1 on success.
 */
static inline int convertToEdge(const char* possibleEdge, edge_t *edge_out)
{
    char *pointerToCharAfterNumber;

    edge_out->firstNodeIndex = strtoumax(possibleEdge, &pointerToCharAfterNumber, 10);
    if (edge_out->firstNodeIndex == UINTMAX_MAX ||
        (edge_out->firstNodeIndex == 0 && possibleEdge[0] != '0'))
        return -1;

    if (pointerToCharAfterNumber[0] != '-')
        return -1;


    char *pointerToSecondNumber = ++pointerToCharAfterNumber;

    edge_out->secondNodeIndex = strtoumax(pointerToSecondNumber, &pointerToCharAfterNumber, 10);
    if (edge_out->secondNodeIndex == UINTMAX_MAX ||
        (edge_out->secondNodeIndex == 0 && pointerToSecondNumber[0] != '0'))
        return -1;

    if (pointerToCharAfterNumber[0] != '\0')
        return -1;

    return 0;
}

/**
 * @brief Adds a new node with the given node index and 'color' 0 to the given array of nodes
 *        (must be initialized with malloc) if it does not already contain a node with this index.
 * @details In order to add a new node, realloc is used to increase the size of the given node array.
 *
 * @param nodeIndex   The index that should be added to the given list of nodes if it wasn't already.
 * @param currentSize The current number of nodes in the given array.
 * @param nodes_out   A pointer to the array of nodes that should contain a node with the specified index after
 *                    the method returns.
 *
 * @return 1 if the array did not contain a node with the given index and it was added, 0 otherwise.
 */
static inline int addNodeIfNew(unsigned long nodeIndex, int currentSize, node_t *nodes_out[])
{
    if (getNodePtr(*nodes_out, currentSize, nodeIndex) != NULL)
        return 0;

    *nodes_out = realloc(*nodes_out, (currentSize+1) * sizeof(node_t));
    if (nodes_out == NULL)
        printErrnoAndTerminate("realloc in addNodeIfNew failed");

    (*nodes_out)[currentSize].nodeIndex = nodeIndex;
    (*nodes_out)[currentSize].color     = 0;

    return 1;
}

/**
 * @brief Converts the program arguments to an array of edges.
 * @details Receives the program arguments, parses them and sets the output parameters accordingly.
 * Terminates the program with EXIT_FAILURE upon any realloc failure or if not all arguments are in the expected format
 * or there are less than 3 arguments, since a valid possibly 3colorable graph must have at least three nodes
 * and therefore three edges.
 *
 * @param argumentCounter   The amount of actual arguments.
 * @param argumentValues    The actual arguments.
 * @param edges_out         A pointer to an array holding all the given edges.
 * @param numberOfEdges_out The number of given edges.
 */
static inline void tryParseArguments(int argumentCounter, char *argumentValues[],
                                     edge_t *edges_out[], int *numberOfEdges_out,
                                     node_t *nodes_out[], int *numberOfNodes_out)
{
    if (argumentCounter < NUMBER_OF_NECESSARY_EDGES+1)
        printErrorAndTerminate(INPUT_ARGUMENT_NUMBER_ERROR);

    *edges_out = malloc(sizeof(edge_t));
    *nodes_out = malloc(sizeof(node_t));

    int i = 1;
    int nodesNum = 0;
    for(; i < argumentCounter; i++)
    {
        if (i != 1) // prevent unnecessary call
        {
            *edges_out = realloc(*edges_out, i * sizeof(edge_t));
            if (edges_out == NULL)
                printErrnoAndTerminate("realloc in tryParseArguments failed");
        }

        if(convertToEdge(argumentValues[i], &((*edges_out)[i - 1])) == -1)
            printErrorAndTerminate(INPUT_ARGUMENT_FORMAT_ERROR);

        edge_t edge  = (*edges_out)[i - 1];
        nodesNum += addNodeIfNew(edge.firstNodeIndex, nodesNum, nodes_out);
        nodesNum += addNodeIfNew(edge.secondNodeIndex, nodesNum, nodes_out);
    }

    *numberOfEdges_out = i - 1;
    *numberOfNodes_out = nodesNum;
}