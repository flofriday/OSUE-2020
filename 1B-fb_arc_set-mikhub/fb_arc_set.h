#ifndef _FB_ARC_SET_H_
#define _FB_ARC_SET_H

/**
 * @file fb_arc_set.h
 * @author Michael Huber 11712763
 * @date 12.11.2020
 * @brief OSUE Exercise 1B fb_arc_set
 * @details Shared header file for declaration of 
 * types and structures used by both the supervisor
 * and the generators.
 */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <limits.h>

#define SHM_NAME "/11712763_shm"
#define BUF_SIZE (25)
#define SEM_FREE "/11712763_free"
#define SEM_USED "/11712763_used"
#define SEM_MUTEX "/11712763_mutex"

/**
 * @brief Represents a vertex by its index.
 * @details Index is stored as long.
 */
typedef long vertex;

/**
 * @brief Represents a directed edge by its two vertices.
 * @details Struct containing start vertex u and end vertex v.
 */
typedef struct {
    /** Start vertex of a directed edge **/
    vertex u;

    /** End vertex of a directed edge **/
    vertex v;

} edge;

/**
 * @brief Container used for storing possible solutions.
 * @details Struct containing a edge-array with a capacity of 8 and a
 * counter for information on how many edges are currently stored in a
 * container. Maximum capacity of 8 is given by specification.
 */
typedef struct {
    /** edge array containing edges stored in the container */
    edge container[8];

    /** Number of edges currently stored in the container */
    size_t counter;

} edge_container;

/**
 * @brief Shared memory for reading and writing possible solutions
 * @details Struct used as shared memory object by supervisor and generators.
 * Central part is an edge_container array which size is defined by BUF_SIZE.
 * Its size is BUF_SIZE*sizeof(edge_container)+16 = BUF_SIZE*136+16, so in the default
 * implementation with BUF_SIZE 25 the size in total is 3416 bytes.
 */
typedef struct {
    /** edge_container array containing possible solutions */
    edge_container data[BUF_SIZE];

    /** Next array position that has not been read yet */
    unsigned int read_pos;

    /** Next free array position to write to (if buffer is not full) */
    unsigned int write_pos;

    /** Flag to initiate shutdown/termination */
    unsigned int terminate;

    /** Number of running generators (important for shutdown) */
    unsigned int num_of_generators;

} circular_buffer;

#endif