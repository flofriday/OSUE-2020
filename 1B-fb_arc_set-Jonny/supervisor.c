/**
 * @file supervisor.c
 * @author Jonny X (12345678) <e12345678@student.tuwien.ac.at>
 * @date 19.11.2020
 *
 * @brief The supervisor program. Is only called once and analyzes the solutions in the circular buffer.
 * 
 * Takes no parameters and continuously reads the circular buffer and outputs the solution if it is the best 
 * solution so far. Terminates if any acyclic graph is found or if an SIGINT/SIGTERM signal is received
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#include "circular_buffer.h"

char *myprog;

volatile sig_atomic_t quit = 0;
void handle_signal(int signal) { quit = 1; }

/** usage function
 * @brief
 * Prints the usage format to explain which arguments are expected.
 * 
 * @details
 * Gets called when an input error by the user is detected.
**/
void usage(void)
{
    fprintf(stderr, "[%s] Usage: %s\n", myprog, myprog);
    exit(EXIT_FAILURE);
}

/** countChar function
 * @brief
 * Counts the occurances of a symbol in a char pointer
 * 
 * @details
 * Counter the occurances by iterating over the char pointer in a loop, looking at each symbol.
**/
int countChar(const char *input, const char symbol)
{
    int occurences = 0;
    for (int i = 0; input[i]; ++i)
    {
        if (input[i] == symbol)
        {
            ++occurences;
        }
    }
    return occurences;
}

/**
 * Program entry point.
 * @brief The program first parses the arguments. Then reads solution until any acyclic graph solution
 * is found or it receives an SIGINT/SIGTERM singal.
 *  
 * @details First it is made sure that the arguments are none. Then signal handling is set up and the circular buffer
 * is opened. Then it contiuously reade the circular buffer and prints the solution if it is the best one so far.
 * When it terminates, it frees up all allocated resources and terminates.
 * 
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char **argv)
{
    myprog = argv[0];

    if (argc > 1)
    {
        usage();
    }

    //setup up signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    char *data;
    shared_memory *shm = open_circular_buffer(true);
    if (shm == NULL)
    {
        fprintf(stderr, "[%s] Error: open_circular_buffer failed: %s\n", myprog, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int minimal = MAX_SOLUTION_EDGE_COUNT + 1;

    while (!quit)
    {
        data = read_circular_buffer(shm);
        if (data == NULL)
        {
            break;
        }
        int occurences = countChar(data, '-');
        if (occurences == 0)
        {
            printf("[%s] The graph is acyclic!\n", myprog);
            free(data);
            break;
        }
        if (occurences < minimal)
        {
            minimal = occurences;
            printf("[%s] Solution with %d edges: %s\n", myprog, occurences, data);
        }
        free(data);
    }

    if (close_circular_buffer(shm, true) == -1)
    {
        fprintf(stderr, "[%s] Error: close_circular_buffer failed: %s\n", myprog, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("\n");

    exit(EXIT_SUCCESS);
}