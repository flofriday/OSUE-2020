/**
 * @file cpair.c
 * @author briemelchen
 * @date 28.11.2020
 * @brief program which computes the closest pair of points in a given array of points.
 * @details the program reads the input line by line from stdin, where each line holds an point
 * x y whitespace seperated. After reading, if their are more then two points, the program
 * forks and starts 2 child processes, which execute the program recursivly using execlp.
 *  The parent-child-communication is implemented through
 * pipes, where the child processes stdin and stdout are redirected.
 *  The parent process then writes into each reading-pipe of the childs:
 * - if x <= mean(x) of a specific point, the first child process handles the point
 * - otherwise the second child handles the process.
 * The child processes then repeat these steps.
 * After that each parent process waits for his child's to finish.
 * If they are finished, it reads the closest point of each part (first/second child) and
 * calculates a third pair: the closest pair between the 2 parts.
 * After that the pair with the closest distance is written to stdout (so parents can read it or for the first 
 * parent to actually print it).
  **/

#include "cpair.h"

/**
 * @brief reads the solution of the part proccessed by a specific child
 * @details reads from the according pipe-end and closes them afterwards.
 * The result of each child is saved in the passed p_pair struct.
 * @param child an integer marking the correct pipe: only 2 values 
 * are allowed: CHILD1_WRITE or CHILD2_WRITE @see cpair.h
 * All other values will make undefined behavior.
 * @param child_index an integer which specifices the index in point_sizes: 
 * should only be 1 or 2 (first/second child), all other values will bring undefined behavior.
 * @param pipes holding the file-descriptor of the pipes.
 * @param p_pair pointer to the point-pair (including the distance between them) where the solution should be stored
 * @param point_counts array holding 3 entries: 
 * [0] -> amount of points of the parent process
 * [1] -> amount of points of first child process
 * [2] -> amount of points of second child process
 * @return 0 on success, -1 if a failure occured.
 **/
static int read_from_child(int child, int child_index, int pipes[4][2], p_pair *p_pair, int point_counts[3]);

/**
 * @brief reads from the child processes, computes the closest pair between their points,
 * compares them and writes the closest pair to stdout.
 * @details waits for the child processes to finish and afterwards read their particular solution of their part.
 * After that, P3, the closest pair of points between the two parts is calculated.
 * Finally, the 3 solutions, closest pair of first-child, closest pair of second point and 
 * closest point between their points are compared and the pair with the shortest distance
 * is written to stdout.
 * @param pid_child1 process id of the first child
 * @param pid_child2 process id of the second child
 * @param total_points pointer to the total points known by the parent
 * @param points_child1 pointer to the points of the first child
 * @param points_child2 pointer to the points of the second child
 * @param point_counts array holding 3 entries: 
 * [0] -> amount of points of the parent process
 * [1] -> amount of points of first child process
 * [2] -> amount of points of second child process
 * @param pipes holding the file-descriptors of the pipes.
 * @return 0 on success, -1 if an error occured.
 **/
static int read_and_compute(pid_t pid_child1, pid_t pid_child2, point *total_points, point *points_child1, point *points_child2,
                            int point_counts[3], int pipes[4][2]);

/**
 * @brief read all points from stdin (might be redirected)
 * @details reads all points and store them into the pointer. Updates array values
 * and calculates the mean over all x values.
 * @param total_points pointer to a pointer of points where the points should be stored.
 * Pointer to pointer is needed here, so realloc can be used in the function
 * @param point_counts array holding 3 entries: 
 * [0] -> amount of points of the parent process (will be updated)
 * [1] -> amount of points of first child process
 * [2] -> amount of points of second child process
 * @param point_sizes array holding 3 entries: 
 * [0] -> size of points able to hold in pointer of the parent process (will be updated)
 * [1] -> size of points able to hold in pointer of first child process
 * [2] -> size of points able to hold in pointer of second child process
 * @param mean pointer to a float, where the mean should be stored. Will hold the calculated mean
 * after method  execution.
 * @return 0 on success, -1 on failure
 **/
static int read_input(point **total_points, int point_counts[3], int point_sizes[3], float *mean);

/**
 * @brief function is used to write points to child processes as parent process using pipes.
 * @details parent writes to the according pipe ends in the following manner:
 * - if x <= mean(x) the point x is written to the first childs pipe
 * - otherwise the point x is written to the second childs pipe.
 * In according pointers, the points of each child are stored additionaly in the parent process.
 * (needed for futher computation)
 * @param total_points pointer to the total points
 * @param points_child1 pointer to a pointer of points, which is used to store the points of the first child.
 * Pointer to pointer is necessary here, so realloc can be peformed inside the function.
 * @param points_child1 pointer to a pointer of points, which is used to store the points of the second child.
 * Pointer to pointer is necessary here, so realloc can be peformed inside the function.
 * @param point_counts array holding 3 entries: 
 * [0] -> amount of points of the parent process
 * [1] -> amount of points of first child process
 * [2] -> amount of points of second child process
 * @param point_sizes array holding 3 entries: 
 * [0] -> size of points able to hold in pointer of the parent process
 * [1] -> size of points able to hold in pointer of first child process
 * [2] -> size of points able to hold in pointer of second child process
 * @param pipes holding the file-descriptors of all points
 * @param mean the mean over all x values in total_points
 * @return 0 on success, -1 in case of failure
 **/
static int parent_write(point *total_points, point **points_child1, point **points_child2,
                        int point_counts[3], int point_sizes[3], int pipes[4][2], float mean);

/**
 * @brief setups pipes
 * @details setups 4 pipes:
 * - pipe where child1 reads from, parent writes to
 * - pipe where child2 reads from, parent writes to
 * - pipe where child1 writes to, parent reads from
 * - pipe where child2 writes to, parent reads from
 * @param pipes 2D array where the pipe filedescritors are stored.
 * @return 0 on success, -1 on failure
 **/
static int setup_pipes(int pipes[4][2]);

/**
 * @brief closes pipes for child-processes and duplicates file descriptors.
 * @details following pipes are closed:
 * - write-end of read pipe for both children, because only parents write to them
 * - read-end of write pipe for both children, because only parents read from them
 * file-descriptors are duplicated:
 * - read-end of read pipe for child is replaced by stdin-fd, so the child reads from the pipe, not stdin.
 * - write-end of write pipe for child is replaced by stdout-fd, so the child writes to the pipe, not stdout.
 * Afterwards, the old fds are closed.
 * @param pipes holding the file-descriptors of the pipes.
 * @param child indicating which child performs the operation. 
 * Only CHILD1_READ and CHILD2_READ @see cpari.h are valid options, all other
 * will indicate undefined behavior.
 * @return 0 on success, -1 in case of an failure.
 **/
static int dup_close_child(int pipes[4][2], int child);

/**
 * @brief closes unnedded pipes for parent proccess.
 * @details following pipe-ends are closed:
 * - read-end of pipe from which child1 reads, and parent writes
 * - read-end of pipe from which child2 reads, and parent writes
 * - write-end of pipe to which child1 writes, and parent reads
 * - write-end of pipe from which child2 writes, and parent reads
 * @param pipes holding the file-descritors of the pipes
 * @return 0 on succcess, -1 in case of failure
 **/
static int close_pipes_parent(int pipes[4][2]);

/**
 * @brief checks if a given string is a valid point as specified.
 * @details checks if using regular expressions (line of 2 floating point numbers, seperated by whitespace)
 * @param str to be checked
 * @return true, if the point is valid, otherwise false
 **/
static bool is_valid_point(const char *str);

/**
 * @brief parses a string into an point struct
 * @details parses the string specified in input to a point.
 * Function should only be called, if is_valid_point has been called onto the same string in advance,
 * otherwise undefined behavior might occur.
 * @param input which should be parsed
 * @return point holding the x and y coordinate of the point
 **/
static point string_to_point(char *input);

/**
 * @brief calculates the distance between 2 points.
 * @details calculates using the pythagorean theorem: sqrt((x2-x1)^2 + (y2-y1)^2)
 * @param p1 first point
 * @param p2 second point
 * @returns the distancce between the 2 given points
 **/
static float dist(point p1, point p2);

/**
 * @brief compute the closest pair between the 2 parts processed by the childs.
 * @details computes the closest pair between 2 parts (mentioned in the assignment as "P3").
 * Iterates through all points of first part and compares them to the second part, if a better 
 * solution is found, the actual return value is overwritten.
 * @param child1_points pointer which points to the points of the first child (first part)
 * @param child2_points pointer which points to the points of the second child (second part)
 * @param point_counts array holding 3 entries: 
 * [0] -> amount of points of the parent process
 * [1] -> amount of points of first child process
 * [2] -> amount of points of second child process
 * @return p_pair containing the closest pair of points between the 2 parts and the distance between them.
 **/
static p_pair compute_closest_pair(point *child1_points, point *child2_points, int point_counts[3]);

/**
 * @brief frees pointer-resources
 * @details frees all passed parameters using free(), if they are not NULL
 * @param p1 pointer to points which should be freed
 * @param p2 pointer to points which should be freed
 * @param p3 pointer to points which should be freed
 **/
static void clean_up(point *p1, point *p2, point *p3);

/**
 * @brief prints the usage message of the program.
 * @details format: [USAGE:] PROGRAM_NAME
 **/
static void usage(void);

/**
 * @brief writes an error message and exits the program.
 * @details the error message should contain a proper message explaining the error.
 * If errno is set, the specific reason explaining errno will also be printed.
 * Exits with exit-code 1.
 * @param error_message holding the reason for the failure.
 **/
static void error(char *error_message);

static char *PROGRAM_NAME; //name of the program, argv[0]

/**
 * @brief starting point of the program.
 * @details defines used structs, arrays and variables:
 *  - total_points: holding all points handled by the process
 *  - child1_points: holding all points of the first child process
 *  - child2_points: holding all points of the first child process
 *  - point_counts: array holding 3 entries: 
 *      [0] -> amount of points of the parent process
 *      [1] -> amount of points of first child process
 *      [2] -> amount of points of second child process
 *  - point_sizes: array holding 3 entries: 
 *      [0] -> size of points able to hold in pointer of the parent process
 *      [1] -> size of points able to hold in pointer of first child process
 *      [2] -> size of points able to hold in pointer of second child process
 *  - mean: mean over all x values.
 * After reading input, fork will be called to start 2 child programs, which will
 * recursivly execute the program. 
 * Calls according methods for parent/child processes.
 * @param argc argument counter, should only be 1
 * @param argv argument vector, should only hold program name
 * @return 0 on success, -1 if a failure occured.
 **/
int main(int argc, char *argv[])
{
    PROGRAM_NAME = argv[0];
    if (argc != 1)
        usage();

    int point_counts[3] = {0, 0, 0};
    int point_sizes[3] = {4, 2, 2}; // init pointers with size 4/2/2
    point *total_points = malloc(sizeof(point) * point_sizes[0]);
    point *child1_points = malloc(sizeof(point) * point_sizes[1]);
    point *child2_points = malloc(sizeof(point) * point_sizes[2]);
    float mean = 0;
    if (total_points == NULL || child1_points == NULL || child2_points == NULL)
    {
        clean_up(total_points, child1_points, child2_points);
        error("memory allocation failed!");
    }

    if (read_input(&total_points, point_counts, point_sizes, &mean) == -1)
    {
        clean_up(total_points, child1_points, child2_points);
        error("failed to read input ");
    }
    if (point_counts[0] <= 1) // only one epoint
    {
        clean_up(total_points, child1_points, child2_points);
        exit(EXIT_SUCCESS);
    }
    else if (point_counts[0] == 2) // two points
    {
        fprintf(stdout, "%f %f\n%f %f\n", total_points[0].x, total_points[0].y, total_points[1].x, total_points[1].y);
        fflush(stdout);
        clean_up(total_points, child1_points, child2_points);
        exit(EXIT_SUCCESS);
    }
    else // > 2 points
    {
        pid_t pid_child1, pid_child2;
        int pipes[4][2];
        if (setup_pipes(pipes) == -1)
        {
            clean_up(total_points, child1_points, child2_points);
            error("failed to setup pipes");
        }

        switch (pid_child1 = fork())
        {
        case -1:
            clean_up(total_points, child1_points, child2_points);
            error("fork failed");
            break;
        case 0:
            if (dup_close_child(pipes, CHILD1_READ) == -1)
            {
                clean_up(total_points, child1_points, child2_points);
                error("failed to dup and/or close pipes!");
            }
            if (execlp(PROGRAM_NAME, PROGRAM_NAME, NULL) == -1)
            {
                clean_up(total_points, child1_points, child2_points);
                error("execlp failed!");
            }
            break;

        default:
            switch (pid_child2 = fork())
            {
            case -1:
                clean_up(total_points, child1_points, child2_points);
                error("fork failed");
                break;
            case 0:
                if (dup_close_child(pipes, CHILD2_READ) == -1)
                {
                    clean_up(total_points, child1_points, child2_points);
                    error("failed to dup and/or close pipes!");
                }
                if (execlp(PROGRAM_NAME, PROGRAM_NAME, NULL) == -1)
                {
                    clean_up(total_points, child1_points, child2_points);
                    error("execlp failed!");
                }
                break;
            default:
                if (close_pipes_parent(pipes) == -1)
                {
                    clean_up(total_points, child1_points, child2_points);
                    error("failed to close pipes for parent!");
                }
                if (parent_write(total_points, &child1_points, &child2_points, point_counts, point_sizes, pipes, mean) == -1)
                {
                    clean_up(total_points, child1_points, child2_points);
                    error("failed to write to child-pipes!");
                }
                if (read_and_compute(pid_child1, pid_child2, total_points, child1_points, child2_points, point_counts, pipes) == -1)
                {
                    clean_up(total_points, child1_points, child2_points);
                    error("failed to read from childs");
                }

                break;
            }
            break;
        }
    }
    clean_up(total_points, child1_points, child2_points);
    exit(EXIT_SUCCESS);
}

/**************************************************READ-PARENT********************************************/
static int read_from_child(int child, int child_index, int pipes[4][2], p_pair *p_pair, int point_counts[3])
{
    FILE *fchild = fdopen(pipes[child][READ], "r");
    if (fchild == NULL)
        return -1;

    char *line = NULL;
    size_t n = 0;
    ssize_t result;
    if (point_counts[child_index] >= 1)
    {
        if ((result = getline(&line, &n, fchild)) != -1)
        {
            point p = string_to_point(line);
            p_pair->p1.x = p.x;
            p_pair->p1.y = p.y;
        }
    }
    if (point_counts[child_index] >= 2) // atleast 2 points are needed to get a valid solution
    {

        if ((result = getline(&line, &n, fchild)) != -1)
        {
            point p = string_to_point(line);
            p_pair->p2.x = p.x;
            p_pair->p2.y = p.y;
        }
        p_pair->dist = dist(p_pair->p1, p_pair->p2);
    }
    free(line);
    if (ferror(fchild) < 0 || fclose(fchild) < 0)
        return -1;
    return 0;
}

static int read_and_compute(pid_t pid_child1, pid_t pid_child2,
                            point *total_points, point *points_child1, point *points_child2,
                            int point_counts[3], int pipes[4][2])
{
    int state;
    while (waitpid(pid_child1, &state, 0) < 0)
    {
        if (errno == EINTR) // interrupted
            continue;
        return -1;
    }

    if (WEXITSTATUS(state) != EXIT_SUCCESS) // first child failed
        return -1;

    while (waitpid(pid_child2, &state, 0) < 0)
    {
        if (errno == EINTR) // interrupted
            continue;
        return -1;
    }
    if (WEXITSTATUS(state) != EXIT_SUCCESS) // second child failed
        return -1;

    p_pair ppair1 = {.p1.x = FLT_MAX, .p1.y = FLT_MAX, .p2.x = FLT_MIN, .p2.y = FLT_MIN, .dist = FLT_MAX};
    p_pair ppair2 = {.p1.x = FLT_MAX, .p1.y = FLT_MAX, .p2.x = FLT_MIN, .p2.y = FLT_MIN, .dist = FLT_MAX};
    p_pair ppair3 = {.p1.x = FLT_MAX, .p1.y = FLT_MAX, .p2.x = FLT_MIN, .p2.y = FLT_MIN, .dist = FLT_MAX};

    if (read_from_child(CHILD1_WRITE, 1, pipes, &ppair1, point_counts) < 0)
        return -1;

    if (read_from_child(CHILD2_WRITE, 2, pipes, &ppair2, point_counts) < 0)
        return -1;

    ppair3 = compute_closest_pair(points_child1, points_child2, point_counts);

    if ((ppair1.dist < ppair2.dist) && (ppair1.dist < ppair3.dist))
        fprintf(stdout, "%f %f\n%f %f\n", ppair1.p1.x, ppair1.p1.y, ppair1.p2.x, ppair1.p2.y);

    else
    {
        if (ppair2.dist < ppair3.dist)
            fprintf(stdout, "%f %f\n%f %f\n", ppair2.p1.x, ppair2.p1.y, ppair2.p2.x, ppair2.p2.y);
        else
            fprintf(stdout, "%f %f\n%f %f\n", ppair3.p1.x, ppair3.p1.y, ppair3.p2.x, ppair3.p2.y);
    }
    fflush(stdout);
    return 0;
}

/***************************************PARENT-WRITE***********************************************/
static int parent_write(point *total_points, point **points_child1, point **points_child2,
                        int point_counts[3], int point_sizes[3], int pipes[4][2], float mean)
{
    FILE *f_child1 = fdopen(pipes[CHILD1_READ][WRITE], "a");
    FILE *f_child2 = fdopen(pipes[CHILD2_READ][WRITE], "a");
    for (size_t i = 0; i < point_counts[0]; i++)
    {
        point actual = total_points[i];
        int needed_space = (snprintf(NULL, 0, "%f %f\n", actual.x, actual.y)) + 1; //size needed for buffer
        char *buffer = malloc(needed_space);
        if (buffer == NULL)
            return -1;
        snprintf(buffer, needed_space, "%f %f\n", actual.x, actual.y);

        if (actual.x <= mean) // write to first child
        {
            point_counts[1]++;
            if (point_counts[1] >= point_sizes[1]) // realloc necessary
            {
                point_sizes[1] *= 2;
                if (((*points_child1) = realloc((*points_child1), (sizeof(point) * point_sizes[1]))) == NULL)
                {
                    free(buffer);
                    return -1;
                }
            }
            if (fprintf(f_child1, "%s", buffer) < 0) // write into pipe
            {
                free(buffer);
                return -1;
            }
            (*points_child1)[point_counts[1] - 1] = actual; // store  point
        }
        else // write to second child
        {
            point_counts[2]++;
            if (point_counts[2] >= point_sizes[2]) // realloc necessary
            {
                point_sizes[2] *= 2;
                if (((*points_child2) = realloc((*points_child2), (sizeof(point) * point_sizes[2]))) == NULL)
                {
                    free(buffer);
                    return -1;
                }
            }
            if (fprintf(f_child2, "%s", buffer) < 0) // write into pipe
            {
                free(buffer);
                return -1;
            }
            (*points_child2)[point_counts[2] - 1] = actual; // store point
        }
        free(buffer);
    }

    if (ferror(f_child1) || ferror(f_child2))
        return -1;

    if (fclose(f_child1) < 0 || fclose(f_child2) < 0) // closing write end of pipes
        return -1;
    return 0;
}
/*********************************************PIPE-SETUP***********************************************/
static int close_pipes_parent(int pipes[4][2])
{
    if (close(pipes[CHILD1_READ][READ]) == -1)
        return -1;
    if (close(pipes[CHILD1_WRITE][WRITE]) == -1)
        return -1;
    if (close(pipes[CHILD2_READ][READ]) == -1)
        return -1;
    if (close(pipes[CHILD2_WRITE][WRITE]) == -1)
        return -1;
    return 0;
}

static int dup_close_child(int pipes[4][2], int child)
{
    if (close(pipes[CHILD1_READ][WRITE]) != 0)
        return -1;
    if (close(pipes[CHILD2_READ][WRITE]) != 0)
        return -1;
    if (close(pipes[CHILD1_WRITE][READ]) != 0)
        return -1;
    if (close(pipes[CHILD2_WRITE][READ]) != 0)
        return -1;
    if (close(pipes[child == 0 ? 2 : 0][READ]) != 0)
        return -1;
    if (close(pipes[child == 0 ? 3 : 1][WRITE]) != 0)
        return -1;
    if ((dup2(pipes[child][READ], STDIN_FILENO)) < 0)
        return -1;
    if ((dup2(pipes[child + 1][WRITE], STDOUT_FILENO)) < 0)
        return -1;
    if (close(pipes[child][READ]) != 0)
        return -1;
    if (close(pipes[child + 1][WRITE]) != 0)
        return -1;
    return 0;
}

static int setup_pipes(int pipes[4][2])
{
    if (pipe(pipes[CHILD1_READ]) == -1)
        return -1;
    if (pipe(pipes[CHILD1_WRITE]) == -1)
        return -1;
    if (pipe(pipes[CHILD2_READ]) == -1)
        return -1;
    if (pipe(pipes[CHILD2_WRITE]) == -1)
        return -1;
    return 0;
}

/****************************************INPUT-READ*********************************************************/
static int read_input(point **total_points, int point_counts[3], int point_sizes[3], float *mean)
{
    char *line = NULL;
    size_t n = 0;
    ssize_t result;
    while ((result = getline(&line, &n, stdin)) != -1)
    {
        if (!is_valid_point(line))
        {
            free(line);
            return -1;
        }
        point p = string_to_point(line);
        point_counts[0]++;
        if (point_counts[0] >= point_sizes[0]) //pointer is to small -> realloc needed
        {
            point_sizes[0] *= 2;
            if (((*total_points) = realloc((*total_points), (sizeof(point) * point_sizes[0]))) == NULL)
            {
                free(line);
                return -1;
            }
        }
        (*total_points)[point_counts[0] - 1] = p;
        (*mean) += (*total_points)[point_counts[0] - 1].x;
    }
    free(line);
    (*mean) /= point_counts[0];
    return 0;
}

/***********************************UTIL*****************************************************/
static p_pair compute_closest_pair(point *child1_points, point *child2_points, int point_counts[3])
{
    p_pair ppair3 = {.p1.x = FLT_MAX, .p1.y = FLT_MAX, .p2.x = FLT_MIN, .p2.y = FLT_MIN, .dist = FLT_MAX};

    for (size_t i = 0; i < point_counts[1]; i++)
    {
        point temp1 = child1_points[i];
        for (size_t j = 0; j < point_counts[2]; j++)
        {
            point temp2 = child2_points[j];
            float new_dist = dist(temp1, temp2);
            if (new_dist <= ppair3.dist) // better solution found
            {
                ppair3.p1 = temp1;
                ppair3.p2 = temp2;
                ppair3.dist = new_dist;
            }
        }
    }
    return ppair3;
}

static float dist(point p1, point p2)
{
    return sqrt((pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2)));
}

static point string_to_point(char *input)
{
    // no error handling, checked in advance
    point p;
    input = strtok(input, " ");
    p.x = strtof(input, NULL);
    input = strtok(NULL, " ");
    p.y = strtof(input, NULL);
    return p;
}

static bool is_valid_point(const char *str)
{
    regex_t regex;
    int return_val;
    if (regcomp(&regex, "^[-+]?[0-9]*.?[0-9]+ [-+]?[0-9]*.?[0-9]+(\n)?$", REG_EXTENDED) != 0)
    {
        return false;
    }
    return_val = regexec(&regex, str, (size_t)0, NULL, 0);
    regfree(&regex);
    return return_val == 0;
}

static void clean_up(point *p1, point *p2, point *p3)
{
    if (p1 != NULL)
        free(p1);
    if (p2 != NULL)
        free(p2);
    if (p3 != NULL)
        free(p3);
}

static void error(char *error_message)
{
    if (errno != 0)
        fprintf(stderr, "[%s] ERROR: %s: %s.\n", PROGRAM_NAME, error_message, strerror(errno));
    else
        fprintf(stderr, "[%s] ERROR: %s.\n", PROGRAM_NAME, error_message);
    exit(EXIT_FAILURE);
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s\n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}
