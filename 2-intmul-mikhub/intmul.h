#ifndef _INTMUL_H_
#define _INTMUL_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>

/** Number of children to fork (always 4 with intmul) */
#define NUM_CHILDREN 4
/** Number of pipe ends needed for children */
#define NUM_PIPE_ENDS 4*NUM_CHILDREN

/** Pipe end indices */
#define READ 0
#define WRITE 1

/** Pipe indices */
#define IN 0
#define OUT 1

/** Child indices */
#define AhBh 0
#define AhBl 1
#define AlBh 2
#define AlBl 3

/* BONUS{ */

/** Width of a tree leaf [Width of "INTMUL(X,Y)" is 11 + 5 Padding] */
#define LEAF_WIDTH 16
/** Width of a column - number of forked children * width of a tree leaf */
#define TREE_COLUMN_WIDTH NUM_CHILDREN*LEAF_WIDTH

/* }BONUS */

/** Pipe end */
typedef int pipe_end_t;

/** Struct containing a pipe */
typedef struct {
    pipe_end_t ends[2];
} pipe_t;

/** Struct containing information about child process */
typedef struct {
    pid_t pid;
    pipe_t pipes[2];
} child_process_t;

/** Struct for storing input numbers */
typedef struct {
    char* A;
    char* B;
} intmul_input_t;

#endif