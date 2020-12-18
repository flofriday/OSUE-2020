/**
 * @file supervisor.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 31.10.202
 *
 * @brief Supervisor program module.
 * 
 * Reads solutions from 1 or more generators processes and prints them to stdout
 * if they are the best yet.
 **/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "circbuf.h"

/**
 * @brief Indicator if the process should stop.
 * @details The type is sig_atomic_t so it is ok to be called from the signal 
 * handler.
 */
volatile sig_atomic_t quit = 0;

/**
 * Signal handler
 * @brief This function only sets the global quit variable to 1 (= true).
 * @details global variables: quit
 * @param signal The singal number (will be ignored)
 */
static void handle_signal(int signal)
{
	quit = 1;
}

/**
 * Supervisors entry point.
 * @brief The complete logic is implemented in this function. First the program
 * creates a shared circular buffer. Then it reads in a loop from the buffer and
 * if a solution in the buffer is the best it has yet seen it will print that 
 * solution to stdout. The loop will only be exited if a signal interrupts the 
 * process or the best solution with 0 edges was found.
 * @details global variables: quit
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS on success and EXIT_FAILURE otherwiese.
 */
int main(int argc, char const *argv[])
{
	// Setup the singal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Check that there are no arguments passed to the program
	if (argc > 1)
	{
		fprintf(stderr, "[%s] ERROR: Too many arguments, the supervisor doesn't accept any arguments.\n", argv[0]);
		fprintf(stderr, "[%s] Usage: %s \n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	// Open the shared circular buffer
	struct circbuf *circbuf;
	circbuf = open_circbuf('s');
	if (circbuf == NULL)
	{
		fprintf(stderr, "[%s] ERROR: Unable to open shared circular buffer: %s\n", argv[0], strerror(errno));
		fprintf(stderr, "[%s] If the file already exist, try: rm -f /dev/shm/*XXXXXXXX*\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Setup variables for the return value and the current minimum
	bool has_min = false;
	int min = -1;

	// Loop till we get signal or find a solution with 0 edges
	while (!quit)
	{
		// Read a solution from the circular buffer
		char *s = read_circbuf(circbuf);
		if (s == NULL)
		{
			break;
		}

		// Count the edges in the just read solution
		size_t len = strlen(s);
		int tmp_min = 0;
		for (int i = 0; i < len; i++)
		{
			if (s[i] == '-')
			{
				tmp_min++;
			}
		}

		// Print the solution if it is the best yet
		if (!has_min || tmp_min < min)
		{
			has_min = true;
			min = tmp_min;

			if (min > 0)
			{
				printf("[%s] Solution with %d edges: %s\n", argv[0], min, s);
			}
			else
			{
				// The solution is the best possible, so we can exit this loop
				// and with that the whole process.
				printf("[%s] The graph is 3-colorable!\n", argv[0]);
				quit = true;
			}
		}

		free(s);
	}

	// Free the resources of the circular buffer
	if (close_circbuf(circbuf, 's') == -1)
	{
		fprintf(stderr, "[%s] ERROR: Unable to close shared circular buffer: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
