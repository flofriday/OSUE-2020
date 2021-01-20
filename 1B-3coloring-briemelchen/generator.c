/**
 * @file generator.c
 * @author briemelchen
 * @date 03.11.2020
 * @brief generators take a graph as argument and calculates solutions to the 3-color-problem (on the passed graph).
 * Afterwards it writes the solution (=removed edges) to the circular buffer.
 * @details generators take the graph as node pairs "-" separates, representing edges of a graph. The graph gets parsed
 * and stored in a graph struct (see globals.h for the struct definition). Afterwards a solution to the problem is computed.
 * (see solver.c) and written to the buffer. The writing process is synchronized by semaphores which are declared in
 * buffer.h.
 * last modified: 14.11.2020
 */
#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <errno.h>

#include <string.h>

#include <string.h>

#include <time.h>

#include <fcntl.h>

#include <sys/mman.h>

#include "solver.h"

#define PROGRAM_NAME "./generator"

/**
 * @brief calculates solutions to find 3-colourings of the graph. Writes them afterwards to the buffer.
 * @details Calculates the solutions using functions in solver.c. Write's are synchronized by semaphores,
 * which are declared in buffer.h. Checks the state of the buffer (see buffer.h) and if it is set to -1 by the supervisor
 * all generators are exiting.
 * @param graph which the solution should be computed of.
 */
void solve_and_write(graph *graph);

/**
 * @brief closes all resources.
 * @details closes all resources (semaphores, shared memory).
 * @return 0 on success, -1 on failure.
 */
static int clean(void);

/**
 * @brief prints an error message to stderr and exits.
 * @details prints an error message to stderr and exits it. If errno is set, the reason will also be printed.
 * Exits the program with EXIT_FAILURE.
 * @param error_message containing a appropriate message explaining the error.
 *
 */
static void error(char *error_message);

/**
 * @brief prints the usage message
 * @details prints the usage message in format Usage ...
 */
static void usage(void);

static int shmfd; // holding the file descriptor of the shared memory

/**
 * Entrypoint of the program.
 * @brief Program starts here. Handles arguments, loads the buffer.
 * @details If invalid edges are passed, the program will exit with an error.
 * set's up all resources.
 * @param argc The argument counter.
 * @param argv The argument vector, holding the edges of the graph.
 * @return  0 on success otherwise error code.
 */
int main(int argc, char **argv)
{
    if (argc == 1)
    {
        usage();
    }

    srand(time(0) * getpid()); // sets random seed for coloring the graph: multiplied with getpid()  to get random seeds for every generator

    if ((shmfd = generator_setup()) == -1)
    {
        error("Failed to setup generator");
    }
    graph *full_graph = malloc(sizeof(graph));
    if (full_graph == NULL)
    {
        error("Memory allocation failed");
    }

    full_graph->edges = malloc(sizeof(edge) * (argc - 1));
    if (full_graph->edges == NULL)
    {
        error("Memory allocation failed");
    }
    full_graph->edge_c = (argc - 1);
    if (parse_graph(argv, full_graph->edges) == -1)
    {
        usage();
    }

    get_node_c(full_graph);
    print_graph(full_graph);
    solve_and_write(full_graph);

    free(full_graph->edges);
    free(full_graph);
}

void solve_and_write(graph *graph)
{
    solution sol;
    while (get_state() == 0) //checks state every iteration to stop, when supervisor indicates it.
    {
        sol = calculate_solution(graph);
        if (sol.removed_edges == -1)
            continue;

        int write_failure = write_solution_buffer(sol);
        if (write_failure == -2) // state has been set to -1
            break;
        if(write_failure < 0)
            error("Failed to write to buffer");
        
    }

    if (get_state() == -2)
    { // error while getting the state
        free(graph->edges);
        free(graph);
        error("Error while getting state");
    }
    else // state is set to -1 -> generator needs to exit
    {
        // triggers other generators to leave, because generators can get stuck while they want to write, if the buffer is full
        // and the supervisor does not read anymore.
       if (sem_post(sem_free) != 0)
            error("Failed to post sem_free");


        free(graph->edges);
        free(graph);
    
        if (clean() == 0)
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            error("Failed to close resources");
        }
    }
}

static int clean(void)
{
    int re_val = 0;
    if (sem_close(sem_used) != 0)
        re_val = -1;
    if (sem_close(sem_free) != 0)
        re_val = -1;
    if (sem_close(sem_w_block) != 0)
        re_val = -1;
    if (munmap(buffer, sizeof(*buffer) != 0))
        re_val = -1;
    return re_val;
}

static void error(char *error_message)
{
    fprintf(stderr, "[%s] ERROR: %s: %s.\n", PROGRAM_NAME, error_message,
            strcmp(strerror(errno), "Success") == 0 ? "Failure" : strerror(errno)); // if an error occurs where erno does not get set, Success is not printed
    exit(EXIT_FAILURE);
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s d-d [[d-d] [d-d]...] where d is an integer.\n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}
