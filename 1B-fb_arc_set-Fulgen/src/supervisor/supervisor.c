/**
 * @file supervisor.c
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Supervisor
 *
 * This module represents the supervisor reading sets and printing the best ones.
 * @details See main.
 **/

#include "../common/error.h"
#include "../common/feedback_arc_set.h"
#include "../common/shared_memory.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

const char *program_name; /**< The program name used for error. **/
static struct shared_memory *memory; /**< The used shared memory instance. Global static to avoid passing it to the signal handler. **/

/**
 * @brief Prints a set together with a status message to stdout
 * @param set A pointer to a valid feedback arc set
 */
static void print_set(struct feedback_arc_set *set)
{
	printf("Found solution with %" PRIu8 " edges: ", set->size);

	for (uint8_t i = 0; i < set->size; ++i)
	{
		printf("%" PRIu32 "-%" PRIu32 " ", set->edges[i].u, set->edges[i].v);
	}

	putchar('\n');
}

/**
 * @brief Handles SIGINT and SIGTERM and calls shared_memory_request_quit in order to terminate this process and the generator processes.
 * @param signal The raised signal. Unused.
 * @details Global variables: memory
 */
static void signal_handler(int signal)
{
	(void) signal;
	shared_memory_request_quit(memory);
}

/**
 * Program entry point.
 * @brief This function first sets program_name to argv[0] for the error function. It then creates a shared memory datastructure instance
 * and proceeds to listen for any feedback arc sets written to it by generators. If the read feedback arc has less edges than the previously
 * read instance or none has been read before, it is printed to stdout and set as the new best. If the edge count is zero or SIGINT or SIGTERM
 * are received, the program terminates.
 * Errors are printed to stderr and cause EXIT_FAILURE to be returned
 * @param argc The argument count
 * @param argv The arguments
 * @return EXIT_SUCCESS on success
 * @return EXIT_FAILURE on failure
 */
int main(int argc, char **argv)
{
	(void) argc;
	program_name = argv[0];

	struct feedback_arc_set best;
	/* There can't be a feedback arc set with more edges than MAX_NUM_EDGES which is always <= UINT8_MAX,
	 * so this works as a default comparision value. Not using MAX_NUM_EDGES + 1 as this would overflow
	 * with MAX_NUM_EDGES == UINT8_MAX
	 */
	best.size = UINT8_MAX;

	struct feedback_arc_set contestant;

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = &signal_handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	if ((memory = shared_memory_create(MATRICULAR_NUMBER, SHM_NAME, true, MAX_FEEDBACK_SETS)) == NULL)
	{
		error("Error creating shared memory: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;

	for (;;)
	{
		if (shared_memory_read_feedback_arc_set(memory, &contestant) == -1)
		{
			if (errno != EINTR)
			{
				error("Error reading feedback arc set: %s", strerror(errno));
				ret = EXIT_FAILURE;
			}

			break;
		}

		if (contestant.size < best.size)
		{
			best = contestant;

			if (best.size == 0)
			{
				puts("Graph is acyclic");
				shared_memory_request_quit(memory);
				break;
			}
			else
			{
				print_set(&contestant);
			}
		}
	}

	shared_memory_destroy(memory);
	return ret;
}
