/**
 * @file generator.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 31.10.202
 *
 * @brief Generator program module.
 * 
 * Generates 3-coloring solutions for a specified graph and submits them to a 
 * supervisor process.
 **/

#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "circbuf.h"

/** 
 * @brief The upper limit of edges in a solution the generator is allowed to
 * submit to the supervisor.
 */
#define MAX_EDGES 8

/**
 * Name of the current program.
 */
static const char *procname;

/**
 * @brief Indicator if the process should stop.
 * @details The type is sig_atomic_t so it is ok to be called from the signal 
 * handler.
 */
static volatile sig_atomic_t quit = 0;

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
 * Vertex structure.
 * @brief A vertex consists of a color and a unique name. Allowed values for the
 * color are 0(red), 1(green), 2(blue), -1(not yet defined).
 */
struct vertex
{
	int8_t color;
	char *name;
};

/**
 * Edge structure.
 * @brief A edge is just a connection between two vertecies, and since the edges
 * have no direction in our graph we don't have to save which one is the start 
 * and the end.
 */
struct edge
{
	struct vertex *v1;
	struct vertex *v2;
};

/**
 * Usage function.
 * @brief This function writes a short usage description to stderr and exits the
 * program.
 * @details Exit the process with EXIT_FAILURE.
 * global variable: procname
 **/
static void usage(void)
{
	fprintf(stderr, "[%s] Usage: %s edge...\n", procname, procname);
	fprintf(stderr, "[%s] Examples:\n", procname);
	fprintf(stderr, "[%s] \t %s 0-1 0-2 1-2\n", procname, procname);
	fprintf(stderr, "[%s] \t %s a-b a-c b-c\n", procname, procname);
	fprintf(stderr, "[%s] \t %s TU-WU TU-BOKU WU-BOKU\n", procname, procname);
	fprintf(stderr, "[%s] \t %s 0-1 0-2 0-3 1-2 1-3 2-3\n", procname, procname);
	exit(EXIT_FAILURE);
}

/**
 * Find a vertex in a list of vertecies.
 * @brief This function finds a vertex with the specified name.
 * @param vertecies A list of vertecies.
 * @param len The length of vertecies.
 * @param target The of the vertex we are searching for.
 * @return If found the pointer to the matching vertex will be returned, 
 * otherwise NULL.
 */
static struct vertex *find_vertex(struct vertex *vertecies, size_t len, char *target)
{
	for (size_t i = 0; i < len; i++)
	{
		if (strcmp(vertecies[i].name, target) == 0)
		{
			return &vertecies[i];
		}
	}
	return NULL;
}

/**
 * Find a edge in a list of edges.
 * @brief This function finds an edge connecting two vertecies.
 * @details This function will return edges of the form v1-v2 as well as v2-v1, 
 * because edges in our graph are undirected and therefore order shouldn't 
 * matter.
 * @param edges A list of edges.
 * @param len The length of edges.
 * @param v1 One of the vertecies connected to the edge.
 * @param v2 The other vertex.
 * @return If found the pointer to the matching edge will be returned, 
 * otherwise NULL.
 */
static struct edge *find_edge(struct edge *edges, size_t len, struct vertex *v1, struct vertex *v2)
{
	// Since our edges don't have any directeion we need to check if
	// v1-v2 is in the list of edges as well as v2-v1
	for (size_t i = 0; i < len; i++)
	{
		if (edges[i].v1 == v1 && edges[i].v2 == v2)
			return &edges[i];

		if (edges[i].v1 == v2 && edges[i].v2 == v1)
			return &edges[i];
	}

	return NULL;
}

#ifdef SLOW_ALGO
/**
 * Color all vertecies randomly.
 * @brief A naive implementation to color all vertecies randomly.
 * @details Before using this function the caller should have made a call to 
 * srand to seed the pseudo random number generator.
 * @param vertecies A list of vertecies.
 * @param len The length of vertecies.
 */
static void color_random(struct vertex *vertecies, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		vertecies[i].color = rand() % 3;
	}
}
#endif

#ifndef SLOW_ALGO
/**
 * Color all vertecies randomly.
 * @brief An optimized implementation to color all vertecies randomly.
 * This function tries, to create higher quality coloring of the graph. 
 * It archieves this by avoiding collisions during the random color asignment 
 * where ever possible (A collision is if two connected vertecies have the 
 * same collor). 
 * This function does this by first setting every color to -1 (undefined). Then
 * it goes over all the edges. If both vertecies in that edge don't have a 
 * color yet, they both get a random (but not the same) color. If only one of 
 * the vertecies has a color the other one will get a non-colliding color
 * randomly choosen.
 * If both colors are already asigned we can check if they do collide and if 
 * so we increase a counter. If the counter is equal to the best solution this 
 * process has ever created, we can stop with the current colloring as we won't 
 * achive a better solution with this configurateion.
 * @details Before using this function the caller should have made a call to 
 * srand to seed the pseudo random number generator.
 * In the case this function returns zero, is it possible that some vertecies 
 * still have the color -1(undefined).
 * @param edges A list of edges.
 * @param edges_len The length of edges.
 * @param vertecies A list of vertecies.
 * @param vertecies_len The length of vertecies.
 * @param limit The best solution this process has ever archived.
 * @return True if creataed coloring is the best this process has ever 
 * archived, otherwise false.
 */
static bool color_random_optimized(struct edge *edges, size_t edges_len, struct vertex *vertecies, size_t vertecies_len, size_t limit)
{
	// Set all colors to -1 so we know which one don't have a color yet
	for (size_t i = 0; i < vertecies_len; i++)
	{
		vertecies[i].color = -1;
	}

	// Now color all vertecies in the edges and try to avoid two vertecies
	// having the same color
	size_t collisions = 0;

	for (size_t i = 0; i < edges_len && collisions < limit; i++)
	{
		struct vertex *v1 = edges[i].v1;
		struct vertex *v2 = edges[i].v2;

		if (v1->color == -1 && v2->color == -1)
		{
			v1->color = rand() % 3;
			v2->color = rand() % 2;
			if (v1->color == v2->color)
				v2->color += 1;
			continue;
		}
		if (v1->color == -1)
		{
			v1->color = rand() % 2;
			if (v1->color == v2->color)
				v1->color += 1;
			continue;
		}
		if (v2->color == -1)
		{
			v2->color = rand() % 2;
			if (v1->color == v2->color)
				v2->color += 1;
			continue;
		}
		// Since we are here, it meas both vertecies are already filled, now
		// we can check if the colors collide with each another.
		if (v1->color == v2->color)
			collisions++;
	}

	if (collisions < limit)
	{
		return true;
	}

	return false;
}
#endif

/**
 * Free allocated resoureces
 * @brief Frees the allocated names inside the vertecies.
 * @details The name of the vertecies should not be used after this call.
 * @param vertecies A list of vertecies.
 * @param len The length of vertecies.
 */
static void clean_up(struct vertex *vertecies, size_t vertecies_len)
{
	// Free the names of the vertecies as we allocated that memory earlier
	for (int i = 0; i < vertecies_len; i++)
	{
		free(vertecies[i].name);
	}
}

/**
 * Generate solutions for the 3-coloring problem.
 * @brief This function implements the logic to how a coloring gets
 * generated and submitted to the shared buffer.
 * @details global variables: quit, procname
 * This function only writes to the shared memeory if it found a new best 
 * coloring, to avoid spaming the buffer.
 * @param edges A list of edges.
 * @param edges_len The length of edges.
 * @param vertecies A list of vertecies.
 * @param vertecies_len The length of vertecies.
 * @return 0 if all operations are successfull, otherwise -1.
 */
static int generate_solutions(struct edge *edges, size_t edges_len, struct vertex *vertecies, size_t vertecies_len)
{
	// Initialize the shared circular buffer
	struct circbuf *circbuf;
	circbuf = open_circbuf('c');
	if (circbuf == NULL)
	{
		fprintf(stderr, "[%s] ERROR: Unable to open the shared circular buffer: %s\n", procname, strerror(errno));
		fprintf(stderr, "[%s] Is the supervisor running?\n", procname);
		return -1;
	}

	// Create the array to save the removed edges in
	size_t removed[edges_len];

	// Seed the reandom number generator with the current time in microseconds
	// cominded with the pid of the current process. The combination is via
	// XOR.
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec ^ getpid());

	// Create a random color asignment, and remove edges so that the 3-coloring
	// is valid.
	size_t max_limit = MAX_EDGES + 1;

	while (circbuf->shm->alive && !quit)
	{
		// Note there are two implementation, to coloring the graph, the one
		// desciped in the asignment and a optimized one.
#ifndef SLOW_ALGO
		// Asign random colors optimized implementation
		bool new_sol = color_random_optimized(edges, edges_len, vertecies, vertecies_len, max_limit);
		if (!new_sol)
		{
			continue;
		}
#else
		// Asign random colors default implementation
		color_random(vertecies, vertecies_len);
#endif

		// Find the edges to remove
		size_t cnt_removed = 0;
		for (size_t i = 0; i < edges_len && cnt_removed < max_limit; i++)
		{
			if (edges[i].v1->color == edges[i].v2->color)
			{
				removed[cnt_removed] = i;
				cnt_removed++;
			}
		}

		// Don't write this solution to the shared buffer if it is too big
		if (cnt_removed >= max_limit)
		{
			continue;
		}

		// Set a new limit
		max_limit = cnt_removed;

		// Calculate the length of the output string
		size_t len = 0;

		len = 2 * cnt_removed + 1;
		for (size_t i = 0; i < cnt_removed; i++)
		{
			len += strlen(edges[removed[i]].v1->name);
			len += strlen(edges[removed[i]].v2->name);
		}

		// Creeate the output string and fill it
		char output[len];

		output[0] = 0;
		for (size_t i = 0; i < cnt_removed; i++)
		{
			strcat(output, edges[removed[i]].v1->name);
			strcat(output, "-");
			strcat(output, edges[removed[i]].v2->name);
			strcat(output, " ");
		}

		// Write the output string
		write_circbuf(circbuf, output);
	}

	// Free allocated resources
	if (close_circbuf(circbuf, 'c') == -1)
	{
		fprintf(stderr, "[%s] ERROR: Unable to close the shared circular buffer: %s\n", procname, strerror(errno));
		return -1;
	}

	return 0;
}

/**
 * Supervisors entry point.
 * @brief This function mostly does setup. The logic to how coloring get
 * generated and submitted to the shared buffer is in generate_solution. 
 * However, this function does argument parsing and creates the datastructures 
 * which are needed to generate solutions efficient.
 * This function calls generate_solution after the parsing is done.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS on success and EXIT_FAILURE otherwiese.
 */
int main(int argc, char const *argv[])
{
	// Save the program name
	procname = argv[0];

	// Check if there are any arguments
	if (argc < 2)
	{
		fprintf(stderr, "[%s] ERROR: No edges provided\n", argv[0]);
		usage();
	}

	// Setup the interupt handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// Create an array for edges and vertecies
	size_t edges_length = 0;
	struct edge edges[argc - 1];
	size_t vertecies_length = 0;
	struct vertex vertecies[(argc - 1) * 2];

	// Parse the arguments and add them to the vertecies and edges
	for (int i = 1; i < argc; i++)
	{
		size_t len = strlen(argv[i]);
		char *arg = malloc(sizeof(char) * (len + 1));

		memset(arg, 0, len + 1);
		strncpy(arg, argv[i], len);
		char *pos_del = strchr(arg, '-');

		// Check if each edge is valid
		if (pos_del == NULL)
		{
			fprintf(stderr, "[%s] ERROR: Argument %d \"%s\" is not a valid edge (no seperator found)\n", argv[0], i, arg);
			clean_up(vertecies, vertecies_length);
			exit(EXIT_FAILURE);
		}
		if (pos_del != strrchr(arg, '-'))
		{
			fprintf(stderr, "[%s] ERROR: Argument %d \"%s\" is not a valid edge (multiple seperators found)\n", argv[0], i, arg);
			clean_up(vertecies, vertecies_length);
			exit(EXIT_FAILURE);
		}
		if (pos_del == arg)
		{
			fprintf(stderr, "[%s] ERROR: Argument %d \"%s\" is not a valid edge (missing first vertex)\n", argv[0], i, arg);
			clean_up(vertecies, vertecies_length);
			exit(EXIT_FAILURE);
		}
		if (pos_del == arg + len - 1)
		{
			fprintf(stderr, "[%s] ERROR: Argument %d \"%s\" is not a valid edge (missing second vertex)\n", argv[0], i, arg);
			clean_up(vertecies, vertecies_length);
			exit(EXIT_FAILURE);
		}
		// Split the argument into both vertex names n1 and n2
		size_t l1 = pos_del - arg;
		char *n1 = malloc(sizeof(char) * (l1 + 1));

		if (n1 == NULL)
		{
			free(arg);
			fprintf(stderr, "[%s] ERROR: Unable to allocate memory for vertecies: %s\n", procname, strerror(errno));
			clean_up(vertecies, vertecies_length);
			exit(EXIT_FAILURE);
		}
		strncpy(n1, arg, l1 + 1);
		n1[l1] = 0;
		size_t l2 = len - ((pos_del - arg) + 1);
		char *n2 = malloc(sizeof(char) * (l2 + 1));

		if (n2 == NULL)
		{
			free(arg);
			fprintf(stderr, "[%s] ERROR: Unable to allocate memory for vertecies: %s\n", procname, strerror(errno));
			free(n1);
			clean_up(vertecies, vertecies_length);
			exit(EXIT_FAILURE);
		}
		strncpy(n2, pos_del + 1, l2);
		n2[l2] = 0;

		// Add the vertecieses of the current edge to the vertecies list if
		// they are not already in them.
		struct vertex *v1 = find_vertex(vertecies, vertecies_length, n1);
		if (v1 == NULL)
		{

			vertecies[vertecies_length].name = n1;
			vertecies[vertecies_length].color = -1;
			v1 = &vertecies[vertecies_length];
			vertecies_length++;
		}
		struct vertex *v2 =
			find_vertex(vertecies, vertecies_length, n2);
		if (v2 == NULL)
		{
			vertecies[vertecies_length].name = n2;
			vertecies[vertecies_length].color = -1;
			v2 = &vertecies[vertecies_length];
			vertecies_length++;
		}
		// Add the edge to the edges if it isn't already in them
		struct edge *e = find_edge(edges, edges_length, v1, v2);

		if (e == NULL)
		{
			edges[edges_length].v1 = v1;
			edges[edges_length].v2 = v2;
			edges_length++;
		}

		free(arg);
	}

	// Generate solution till the supervisor kills us or an error occours or
	// the user terminates us.
	int res = generate_solutions(edges, edges_length, vertecies,
								 vertecies_length);

	// Free the resources and exit with the correct code
	clean_up(vertecies, vertecies_length);
	if (res == 0)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
