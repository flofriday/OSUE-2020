/**
 * @author briemelchen
 * @file cpair.h
 * @date 28.11.2020
 * @brief module-header of cpair, which defines macros and structs.
 * @details defines macros, which are used as indices for the pipes @see cpair.c.
 * Furthermore, structs for storing points and pair of points are defined.
 **/

#ifndef cpair_h
#define cpair_h

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
#include <stdbool.h>

#define CHILD1_READ 0  // index of pipe array for child1-read-pipe
#define CHILD1_WRITE 1 // index of pipe array for child1-write-pipe
#define CHILD2_READ 2  // index of pipe array for child2-read-pipe
#define CHILD2_WRITE 3 // index of pipe array for child2-write-pipe
#define READ 0         // helper macro for read position in a pipe-fd-array
#define WRITE 1        // helper macro for write position in a pipe-fd-array

typedef struct point // holds a 2D-Point
{
    float x; // x-coordinate of the point
    float y; // y-coordinate of the point
} point;

typedef struct p_pair // holds a pair of point and the distance between them
{
    point p1; // first point
    point p2; // second point
    float dist; // distance between p1 and p2
} p_pair;

#endif // cpair_h