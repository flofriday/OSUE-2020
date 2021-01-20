/**
 *@file supervisor.c
 * @author briemelchen
 * @date 03.11.2020
 *
 * @brief Supervisor program reads solutions of the 3-Color Problem and Prints them. Furthermore it's supervises
 * multiple generator processes and forces to exit them if a solution is found (or the supervisor terminates).
 *
 * @details The supervisor program does not take any options or arguments. It setups the circular buffer (see buffer.h/buffer.c)
 * and reads from it. The read process is synchronized by semaphores (see buffer.h/buffer.c). The value saved in the buffer are
 * possible solutions to the 3-Color-Problem of graphs, which are given and solved  by generator-processes.
 * The program also handles common signals (SIGINT, SIGTERM) and informs generator processes if the program needs to exit (or if it has found a solution).
 */
#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <errno.h>

#include <string.h>

#include <string.h>

#include <fcntl.h>

#include <limits.h>

#include <fcntl.h>

#include <sys/mman.h>

#include <signal.h>

#include "solver.h"

#define PROGRAM_NAME "./supervisor"

/**
 * @brief reads and prints values from the buffer.
 * @details Reads continuously from the Buffer(if there are any elements to read from) and prints them.
 * New solutions are only printed, if it is a better solution than the currently best solution.
 * If the program is aborted (SIGINT,SIGTERM) or terminates (because a optimal solution has been found) the
 * reading processe aborts and the generators are informed to close aswell.
 */
static void read_and_print_cbuffer(void);

/**
 * @brief signal handler
 * @details signals SIGTERM and SIGINT are handled properly and invoke clearing of shared resources and semaphores, and
 * informates generators.
 * @param  signal to be handled
 */
static void handle_signal(int signal);

/**
 * @brief closes all resources.
 * @details closes all resources (semaphores, shared memory) and unlinks them.
 * @return 0 on success, -1 on failure.
 */
static int clean(void);

/**
 * @brief calls clean and exits the program.
 * @details calls clean() and on successfully calling it exits with EXIT_SUCCESS, on failure,
 * with EXIT_FAILURE
 */
static void clean_exit(void);

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

static int shmfd;                  // holding the filedescriptor of the shared memory
static volatile sig_atomic_t quit; // flag which is set if the program needs to quit due to a signal

/**
 * Entrypoint of the program.
 * @brief Program starts here. Handles arguments, setups the buffer and the signalhandler.
 * @details If any arguments are given, program exits with an error.
 * global-variables: shmfd, quit
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return  0 on success otherwise error code.
 */
int main(int argc, char **argv)
{

    if (argc != 1)
        usage();

    if ((shmfd = supervisor_setup()) == -1)
        error("Error while doing buffer setup for supervisor.");

    quit = 0;

    struct sigaction sig_handler;
    memset(&sig_handler, 0, sizeof(sig_handler));
    sig_handler.sa_handler = handle_signal;

    sigaction(SIGINT, &sig_handler, NULL);
    sigaction(SIGTERM, &sig_handler, NULL);

    read_and_print_cbuffer();
}

static void handle_signal(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        quit++; // because exit is not async-safe i inc the quit flag, which let's the supervisor return gracefully
    }
}

static void read_and_print_cbuffer(void)
{
    int best_sol = INT_MAX; //holds the count of best actual solution

    while (quit == 0)
    {

        /*
    // for debug used:
    int a;
    sem_getvalue(sem_used,&a);
    int b;
    sem_getvalue(sem_free,&b);
    printf("SEMUSED: %d \n",a);
    printf("SEMFREE: %d\n",b);

    */
        solution sol = read_entry_from_buffer();

        if (sol.removed_edges == -1) //-1 indicates error while reading
        {
            if (errno == EINTR) // signal
                break;
            else
                error("Failed reading from buffer");
        }

        if (sol.removed_edges == 0)
        { // optimal solution is found
            best_sol = 0;
            print_solution(sol);

            if (set_state(-1) != 0)
            {
                if (errno == EINTR) // signal
                    break;
                error("Failed to update state of buffer");
            }

            clean_exit();
            return;
        }
        else if (sol.removed_edges < best_sol)
        { // better solution is found
            best_sol = sol.removed_edges;
            print_solution(sol);
        }
    }
    if (set_state(-1) != 0)
        error("Failed to update state of buffer");
    clean_exit();
}

static void clean_exit(void)
{

    if (clean() == 0)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        error("Failed to clean resources.");
    }
}

static void error(char *error_message)
{
    fprintf(stderr, "[%s] ERROR: %s: %s.\n", PROGRAM_NAME, error_message,
            strcmp(strerror(errno), "Success") == 0 ? "Failure" : strerror(errno));
    exit(EXIT_FAILURE);
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s\n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}

static int clean(void)
{
    int re_val = 0;
    // freeing stucked generators, which can stuck, if the buffer is full and they wait for free space to write.
    if (sem_post(sem_free) != 0)
        error("Error while sem_post(sem_w)");


    // printf("Buffer readpos at end: %d\n", buffer->read_pos);
    // printf("Buffer writepos at end: %d\n", buffer->write_pos);
    if (sem_close(sem_free) != 0)
        re_val = -1;
    if (sem_close(sem_used) != 0)
        re_val = -1;
    if (sem_close(sem_w_block) != 0)
        re_val = -1;
    if (sem_unlink(SEM_NAME_WR) != 0)
        re_val = -1;
    if (sem_unlink(SEM_NAME_RD) != 0)
        re_val = -1;
    if (sem_unlink(SEM_NAME_WR_BLOCK) != 0)
        re_val = -1;
    if (shm_unlink(SHM_NAME) != 0)
        re_val = -1;
    if (munmap(buffer, sizeof(*buffer)) != 0)
        re_val = -1;
    return re_val;
}