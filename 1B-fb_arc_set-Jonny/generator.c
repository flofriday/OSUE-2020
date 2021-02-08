/**
 * @file generator.c
 * @author Jonny X (12345678) <e12345678@student.tuwien.ac.at>
 * @date 19.11.2020
 *
 * @brief The generator program. The program continuously generates new solutions and writes them into the circular buffer.
 * 
 * Given the edges as parameters this module continuously generates solutions for the circular buffer and writes them 
 * once there is free space and no other generator is writing.
 * 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

#include "circular_buffer.h"

//global program name
char *myprog;

/**
 * typedef struct Edge.
 * @brief A struct to hold the information from an Edge
 *  
 * @details Consits of 2 ints. One start vertice and one end vertice.
 * 
 */
typedef struct Edge
{
    int start;
    int end;
} edge;

void usage(void)
{
    fprintf(stderr, "[%s] Usage: %s edge...\n", myprog, myprog);
    exit(EXIT_FAILURE);
}

/**
 * set_random function.
 * @brief Sets the random seed.
 *  
 * @details Should be called once at the beginning. The pid and time are used to set a seed for each generator. As multiple
 * generators could be started at the same time, the time is not necessarily enough to make each generator come up with custom solution.
 */
void set_random(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec * getpid());
}

/**
 * get_max function.
 * @brief Finds the largest edge.
 *  
 * @details Finds the largest edge. It assume that the edges are numbered continuously and that the are no gaps.
 * 
 * @param edges Pointer to all parsed input edges.
 * @param length the total amount of edges.
 *
 * @return the maximum edge value
 */
int get_max(edge *edges, int length)
{
    int max = 0;
    for (int i = 0; i < length; ++i)
    {
        if (edges[i].start > max)
        {
            max = edges[i].start;
        }
        if (edges[i].end > max)
        {
            max = edges[i].end;
        }
    }

    return max + 1;
}

/**
 * get_permutation function.
 * @brief Generates a new random permutation.
 *  
 * @details Called each time the generator wants a new permutation to generate a new solution.
 * 
 * @param vertices All vertices, which the permutation should include. The vertices pointer contains the new permutation
 * after functions finishes.
 * @param vertices_count the length of all vertices.
 */
void get_permutation(int *vertices, int vertices_count)
{
    for (int i = 0; i < vertices_count; ++i)
    {
        vertices[i] = i;
    }

    for (int i = 0; i < vertices_count; ++i)
    {
        int j = (rand() % (vertices_count - i)) + i;

        int temp = vertices[i];
        vertices[i] = vertices[j];
        vertices[j] = temp;
    }
}

/**
 * get_index function.
 * @brief Returns the index of the target pemutation element.
 *  
 * @details Iterates through the permutations to find the index of the target element.
 * 
 * @param permutations The permutations which should be iterated through.
 * @param vertices_count The maximum amount of vertices to go though
 * @param target The target element which is compared to each element in permutations
 * @return Returns the index if the element was found. Otherwise returns -1.
 */
int get_index(int *permutations, int vertices_count, int target)
{
    for (int i = 0; i < vertices_count; ++i)
    {
        if (permutations[i] == target)
        {
            return i;
        }
    }

    return -1;
}

/**
 * Program entry point.
 * @brief The program first parses the arguments (Edges) and saves them. Then it continously generates solutions
 * and writes them to the circular buffer.
 *  
 * @details First the circular buffer is opened. After that it is made sure that the specified edges are correct. Otherwise a usage message
 * is printed. After the edges are parsed the generator continously generates solutions and writes them to the circular buffer if the solution
 * is somewhat reasonable (<= 8 edges). The generator also keeps tracks of its own minimum to make sure that only solutions which are lower than
 * the current minimum are written to the buffer.
 * 
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char **argv)
{
    myprog = argv[0];

    shared_memory *shm = open_circular_buffer(false);
    if (shm == NULL)
    {
        return EXIT_FAILURE;
    }

    int edge_count = argc - 1;
    edge edges[edge_count];

    int solutions_counter = 0;
    int solutions[edge_count];

    for (int i = 1; i < argc; ++i)
    {
        char *input = argv[i];
        char *endpointer = NULL;

        edges[i - 1].start = strtol(strtok(input, "-"), &endpointer, 10);
        if (*endpointer != '\0')
        {
            fprintf(stderr, "[%s] Error: Invalid symbol detected: %c\n", myprog, *endpointer);
            usage();
        }

        edges[i - 1].end = strtol(strtok(NULL, ""), &endpointer, 10);
        if (*endpointer != '\0')
        {
            fprintf(stderr, "[%s] Error: Invalid symbol detected: %c\n", myprog, *endpointer);
            usage();
        }

        if (errno == EINVAL)
        {
            usage();
        }

        //printf("Edge: %d-%d\n", edges[i - 1].start, edges[i - 1].end);
    }

    int vertices_count = get_max(edges, argc - 1);
    set_random();
    int permutations[vertices_count];

    int min = MAX_SOLUTION_EDGE_COUNT;
    while (shm->generators_should_quit == false)
    {
        solutions_counter = 0;

        get_permutation(permutations, vertices_count);

        for (int i = 0; i < edge_count; ++i)
        {
            int u = get_index(permutations, vertices_count, edges[i].start);
            int v = get_index(permutations, vertices_count, edges[i].end);

            if (u == -1 || v == -1)
            {
                exit(-1);
            }

            if (u > v)
            {
                solutions[solutions_counter] = i;
                ++solutions_counter;
            }
        }

        if (solutions_counter > min)
        {
            continue;
        }

        min = solutions_counter;

        //calcualte the toal length
        int length = 2 * solutions_counter + 1;
        for (int i = 0; i < solutions_counter; ++i)
        {
            char edge1[8];
            char edge2[8];

            memset(edge1, 0, 8);
            memset(edge2, 0, 8);

            sprintf(edge1, "%d", edges[solutions[i]].start);
            sprintf(edge2, "%d", edges[solutions[i]].end);

            length += strlen(edge1);
            length += strlen(edge2);
        }

        char output[length];
        output[0] = 0;

        for (int i = 0; i < solutions_counter; ++i)
        {
            char edge1[8];
            char edge2[8];
            sprintf(edge1, "%d", edges[solutions[i]].start);
            sprintf(edge2, "%d", edges[solutions[i]].end);

            strcat(output, edge1);
            strcat(output, "-");
            strcat(output, edge2);
            strcat(output, " ");
            //printf("%s-%s\n", edge1, edge2);
        }

        if (write_circular_buffer(shm, output) == -1)
        {
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}