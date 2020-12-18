#include "../common/error.h"
#include "../common/feedback_arc_set.h"
#include "../common/shared_memory.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const char *program_name;

static void fisher_yates_shuffle(uint32_t *vertices, size_t size)
{
	for (size_t i = size - 1; i >= 1; --i)
	{
		size_t j = random() % (i + 1);
		uint32_t temp = vertices[i];
		vertices[i] = vertices[j];
		vertices[j] = temp;
	}
}

static void seed_random()
{
	/* /dev/urandom provides a better entropy source. Use it if available. */
	int fd;
	if ((fd = open("/dev/urandom", O_RDONLY)) > -1)
	{
		unsigned int seed;
		if (read(fd, &seed, sizeof(seed)) > -1)
		{
			srandom(seed);
			close(fd);
			return;
		}

		close(fd);
	}

	/* Fall back to a weak entropy source which might clash with other generators. */
	srandom(time(NULL));
}

bool contains_vertex(uint32_t *vertices, size_t size, uint32_t vertex)
{
	/* Do not use memmem here, as bytes from two contained vertices might
	 * accidentally match the needle vertex, returning a false positive. */

	for (size_t i = 0; i < size; ++i)
	{
		if (vertices[i] == vertex)
		{
			return true;
		}
	}

	return false;
}

int main(int argc, char **argv)
{
	program_name = argv[0];
	if (argc == 1)
	{
		error("No edges supplied");
		return EXIT_FAILURE;
	}

	struct shared_memory *memory = NULL;

	if ((memory = shared_memory_create(MATRICULAR_NUMBER, SHM_NAME, false, MAX_FEEDBACK_SETS)) == NULL)
	{
		error("Failed to open shared memory: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;
	struct edge *edges;
	struct feedback_arc_set feedback_arc_set;
	uint32_t *vertices;
	size_t num_vertices = 0;
	const size_t num_edges = (size_t) argc - 1;

	if ((edges = malloc((argc - 1) * sizeof(struct edge))) == NULL)
	{
		error("edges: malloc failed: %s", strerror(errno));
		ret = EXIT_FAILURE;
		goto cleanup_memory;
	}

	if ((vertices = malloc((argc - 1) * 2 * sizeof(uint32_t))) == NULL)
	{
		error("vertices: malloc failed: %s", strerror(errno));
		ret = EXIT_FAILURE;
		goto cleanup_edges;
	}

	for (int i = 1; i < argc; ++i)
	{
		uint32_t u, v;
		if (sscanf(argv[i], "%" SCNu32 "-%" SCNu32, &u, &v) < 2)
		{
			error("Error parsing argument %d: '%s'", i, argv[i]);
			ret = EXIT_FAILURE;
			goto cleanup_vertices;
		}

		edges[i - 1].u = u;
		edges[i - 1].v = v;

		/* Make sure all vertices are only contained once */

		if (!contains_vertex(vertices, num_vertices, u))
		{
			vertices[num_vertices++] = u;
		}

		if (!contains_vertex(vertices, num_vertices, v))
		{
			vertices[num_vertices++] = v;
		}
	}

	seed_random();

	for (;;)
	{
		if (shared_memory_quit_requested(memory))
		{
			puts("Quit requested");
			break;
		}

		memset(&feedback_arc_set, 0, sizeof(feedback_arc_set));
//#define TEST
#ifdef TEST
		(void) fisher_yates_shuffle;
		uint32_t v[] = {3, 6, 1, 5, 0, 2, 4};
		memcpy(vertices, v, sizeof(v));
		assert(sizeof(v) / sizeof(uint32_t) == num_vertices);
#else
		fisher_yates_shuffle(vertices, num_vertices);
#endif

		for (size_t i = 0; i < num_edges; ++i)
		{
			bool found_v = false;
			for (size_t j = 0; j < num_vertices; ++j)
			{
				if (!found_v)
				{
					if (vertices[j] == edges[i].v)
					{
						found_v = true;
					}
				}

				else if (vertices[j] == edges[i].u)
				{
					/* More edges than allowed by the limit? Don't send it to the supervisor */
					if (feedback_arc_set.size == sizeof(feedback_arc_set.edges) / sizeof(feedback_arc_set.edges[0]))
					{
						goto skip;
					}

					feedback_arc_set.edges[feedback_arc_set.size++] = edges[i];
					break;
				}
			}
		}

#ifdef TEST
		for (uint8_t i = 0; i < feedback_arc_set.size; ++i)
		{
			printf("%" PRIu32 "-%" PRIu32 " ", feedback_arc_set.edges[i].u, feedback_arc_set.edges[i].v);
		}
		break;
#else
		if (shared_memory_write_feedback_arc_set(memory, &feedback_arc_set) == -1)
		{
			if (errno != EINTR)
			{
				error("Error writing feedback arc set: %s", strerror(errno));
				ret = EXIT_FAILURE;
			}

			break;
		}

	skip:
		continue;
#endif
	}


cleanup_vertices:
	free(vertices);
cleanup_edges:
	free(edges);
cleanup_memory:
	shared_memory_destroy(memory);
	return ret;
}
