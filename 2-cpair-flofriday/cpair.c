/**
 * @file cpair.c
 * @author flofriday <eXXXXXXXX@students.tuwien.ac.at>
 * date: 27.11.2020
 * 
 * @brief Complete Implementation of the cpair task.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/**
 * @brief The name of the executing program
 */
const char *procname;

/**
 * Structure of a point
 * @brief The strucutre of a point in two dimensional space.
 */
typedef struct Point
{
    float x;
    float y;
} Point;

/**
 * Remove newline.
 * @brief If the last character is a newline it will be removed.
 * @param line The line string to remove the newline from.
 */
void strip_newline(char *line)
{
    size_t len = strlen(line);
    if (line[len - 1] == '\n')
    {
        line[len - 1] = '\0';
    }
}

/**
 * Write a point to a file.
 * @brief Writing a point to a file in a way that is insured to work with 
 * `parse_point`.
 * @param file The file the point will be written two.
 * @param point A pointer to the point that will be converted to text and 
 * written to the file.
 * @return Upon sucess the number of bytes written is returned. In the case of 
 * a failure a negative number is returned.
 */
int write_point(FILE *file, Point *point)
{
    return fprintf(file, "%f %f\n", point->x, point->y);
}

/**
 * Parse a point from text.
 * @brief Parsing a line with two space seperated floating point numbers into a 
 * point.
 * @details The data of the dst might be changed even if the function returns
 * with an error.
 * @param dst A pointer to the point into which the data will be parsed
 * @param line A pointer to the beginning of the string that will be parsed
 * @return Upon success 0 is returned, otherwise -1.
 */
int parse_point(Point *dst, char *line)
{
    char *pos = line;
    char *endptr;

    // Delete the newlise if the last character is one.
    strip_newline(line);

    // Parse the x-value
    dst->x = strtof(pos, &endptr);
    if (pos == endptr || *endptr != ' ')
    {
        // Either no number was found or the nuber was not followed by
        // a space.
        return -1;
    }

    // Parse the y-value
    pos = endptr + 1;
    dst->y = strtof(pos, &endptr);
    if (pos == endptr || strlen(line) + line != endptr)
    {
        // Either no number was found or, the second number was not the end of
        // the line.
        return -1;
    }

    // Everything worked fine.
    return 0;
}

/**
 * Calculate the distance between two points
 * @brief Calculates the euclidean distance between two Points p1 and p2.
 * @param p1 A pointer to the first point.
 * @param p2 A pointer to the second point.
 * @return The distance between the points.
 */
float calc_distance(Point *p1, Point *p2)
{
    float a = p2->x - p1->x;
    float b = p2->y - p1->y;
    float c = sqrt(a * a + b * b);
    return c;
}

/** Calculate the arithmetic mean.
 * @brief Calculates the arithmetic of the x-value of all points provided.
 * @param points An array with all points.
 * @param len The number of elements in points
 * @return The mean.
 */
float arithmetic_mean(Point *points, size_t len)
{
    double res = 0;
    for (size_t i = 0; i < len; i++)
    {
        res += points[i].x;
    }
    res /= len;
    return (float)res;
}

/**
 * Parse points from stdin
 * @brief Parse the input of stdin. This function will allocate an array for the
 * points it parses and will adjust poinst so that points will be a pointer to
 * that array.
 * @details In case of an error this function writes an error message to 
 * stderr.
 * The caller must free the the array to which points points to.
 * @param points A pointer to an (empty) Point array.
 * @param len The number of the parsed points.
 * @return Upon success 0, otherwise -1.
 */
int parse_stdin(Point **points, size_t *len)
{
    // Create an dynamic array to store the points in.
    size_t cap = 2;
    *points = malloc(sizeof(Point) * cap);
    if (points == NULL)
    {
        fprintf(stderr, "[%s] ERROR: Unable to allocate memmory: %s\n",
                procname, strerror(errno));
        return -1;
    }

    // Read stdin line by line.
    char *line = NULL;
    size_t linecap = 0;
    while (getline(&line, &linecap, stdin) != -1)
    {
        // Resize the array if it isn't big enough
        if (cap == *len)
        {
            cap *= 2;
            Point *tmp = realloc(*points, sizeof(Point) * cap);
            if (tmp == NULL)
            {
                free(line);
                free(*points);
                fprintf(stderr, "[%s] ERROR: Unable to allocate memmory: %s\n",
                        procname, strerror(errno));
                return -1;
            }
            *points = tmp;
        }

        // Parse the point into the array
        if (parse_point(&((*points)[*len]), line) != 0)
        {
            fprintf(stderr, "[%s] ERROR: Cannot parse line %lu \"%s\"\n",
                    procname, *len, line);
            free(line);
            free(*points);
            return -1;
        }

        // Adjust the values for the next loop
        (*len)++;
    }

    // Free the line and return with a success value.
    free(line);
    return 0;
}

/**
 * Parse the input from a child
 * @brief Parse the point output of (only first 2 lines) of the child. 
 * The result of the child will be parsed and saved in the first and 
 * second elements of the points array.
 * @param file A file to read the output of the child.
 * @param points A pointer to the first element in an array with at least two
 * elemnts.
 * @return The number of correctly parsed lines.
 */
int parse_child(FILE *file, Point *points)
{
    char *line = NULL;
    size_t cap = 0;

    // Parse the first line
    if (getline(&line, &cap, file) == -1)
    {
        return 0;
    }
    if (parse_point(&points[0], line) == -1)
    {
        free(line);
        return 0;
    }

    // Parse the second line
    if (getline(&line, &cap, file) == -1)
    {
        free(line);
        return 1;
    }
    if (parse_point(&points[1], line) == -1)
    {
        free(line);
        return 1;
    }

    free(line);
    return 2;
}

/**
 * Parse the output of both children.
 * @brief Parses the output (at most the first two lines, not the tree) of both
 * children. The better result of both children will be saved in the first and 
 * and second field of the points array.
 * @param lfile The file to read the output of the first child.
 * @param rfile The file to read the output of the second child.
 * @param points An array of points with at least the length of two to store
 * the best result of the children.
 * @return Upon success 0, otherwise -1.
 */
int parse_children(FILE *lfile, FILE *rfile, Point *points)
{
    Point left_res[2];
    Point right_res[2];

    // Parse the input of the each child
    int left_len = parse_child(lfile, left_res);
    int right_len = parse_child(rfile, right_res);

    // Find out which child has the better result
    if (left_len == 0 && right_len == 0)
    {
        return -1;
    }
    else if (left_len == 0)
    {
        // The left child retuned with no result so the right must have one
        points[0] = right_res[0];
        points[1] = right_res[1];
        return 0;
    }
    else if (right_len == 0)
    {
        // The right child returned with no result so the left must have one
        points[0] = left_res[0];
        points[1] = left_res[1];
        return 0;
    }

    // Both children have a result so we need to find out which one has the
    // best result.
    if (calc_distance(&right_res[0], &right_res[1]) <=
        calc_distance(&left_res[0], &left_res[1]))
    {
        points[0] = right_res[0];
        points[1] = right_res[1];
        return 0;
    }
    else
    {
        points[0] = left_res[0];
        points[1] = left_res[1];
        return 0;
    }
}

/**
 * Merge the two outputs of both children 
 * @brief Both children found the closest pair in their half. However, there
 * might be a pair of points that is even closer where each point is in a 
 * different half. Therefore this function checks if such a pair exists and if
 * so saves them in p1 and p2.
 * @param points An array of all points.
 * @param len The number of elements in points.
 * @param mean The arithmetic mean of all points
 * @param p1 A pointer to a pointer to the first point of the currently best
 * pair.
 * @param p2 A pointer to a pointer to the first point of the currently best
 * pair.
 */
void merge(Point *points, size_t len, float mean, Point **p1, Point **p2)
{
    // Delta is the currently shortes distance
    float delta = calc_distance(*p1, *p2);

    // Calculate all points on the right side which are candidates (they are
    // close enough to the border).
    Point *right_close[len];
    size_t cnt_close = 0;
    for (size_t i = 0; i < len; i++)
    {
        // Point needs to be right of the mean and cannot be more than delta
        // away from the mean (which is the border between the childs).
        if (points[i].x > mean && points[i].x - mean < delta)
        {
            right_close[cnt_close] = &points[i];
            cnt_close++;
        }
    }

    for (size_t i1 = 0; i1 < len / 2; i1++)
    {
        // Ignore all right points and left points that are too far from
        // the border
        if (points[i1].x > mean || mean - points[i1].x > delta)
        {
            continue;
        }

        // Since we found a left candidate, loop over all right candidates and
        // check if there is a better result than currently in delta.
        for (size_t i2 = 0; i2 < cnt_close; i2++)
        {
            float tmp_delta = calc_distance(&points[i1], right_close[i2]);
            if (tmp_delta < delta)
            {
                // Found a new better pair, so update delta and p1, p2.
                delta = tmp_delta;
                *p1 = &points[i1];
                *p2 = right_close[i2];
            }
        }
    }
}

/**
 * Print a padding.
 * @brief Print spaces to stdout.
 * @param n THe number of spaces to be printed.
 */
void print_padding(size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        putchar(' ');
    }
}

/**
 * Print the branchline.
 * @brief Prints the branchline for the tree to stdout. The first branch
 * (a simple slash) will be printed at llen/2 and the second at (rlen/2) + 2.
 * The reason for the +2 is that there is a two space buffer between the left 
 * and right line.
 * @param llen The length of the left string.
 * @param rlen The length of the right string.
 */
void print_branchline(size_t llen, size_t rlen)
{
    for (size_t i = 0; i < llen + rlen + 2; i++)
    {
        if (i == llen / 2)
        {
            putchar('/');
        }
        else if (i == (rlen / 2) + llen + 2)
        {
            putchar('\\');
        }
        else
        {
            putchar(' ');
        }
    }
    putchar('\n');
}

/**
 * Print the tree.
 * @brief Print the call-tree of this process to stdout.
 * @details lf and rf streams must be pointed at either the first line of the
 * tree or empty line between the result and the tree.
 * lf and rf can be set to NULL, if points_len is less or equal to two.
 * @param points An array of all points of this process.
 * @param points_len The number of elements in points.
 * @param lf The file of the output of the first child.
 * @param rf The file of the output of the second child.
 * @return Upon success 0, otherwise -1.
 */
int print_tree(Point *points, size_t points_len, FILE *lf, FILE *rf)
{
    // Special case with just 1 point
    if (points_len == 1)
    {
        printf("\nCPAIR({%.1f, %.1f})\n", points[0].x, points[0].y);
        return 0;
    }

    // Special case with just 2 points
    if (points_len == 2)
    {
        printf("\nCPAIR({%.1f, %.1f}, {%.1f, %.1f})\n",
               points[0].x, points[0].y, points[1].x, points[1].y);
        return 0;
    }

    // create variables for the left and right line
    char *lline = malloc(sizeof(char) * 2);
    size_t lcap = 2;
    char *rline = malloc(sizeof(char) * 2);
    size_t rcap = 2;

    // Read from both children until we read the first line of the tree
    while (true)
    {
        if (getline(&lline, &lcap, lf) == -1)
        {
            fprintf(stderr,
                    "[%s] ERROR: Creating the tree \
                    (left child didn't create any output)\n",
                    procname);
            free(lline);
            free(rline);
            return -1;
        }
        if (strcmp(lline, "\n") != 0)
        {
            break;
        }
    }
    while (true)
    {
        if (getline(&rline, &rcap, rf) == -1)
        {
            fprintf(stderr, "[%s] ERROR: Creating the tree\
            (right child didn't create any output)\n",
                    procname);
            free(lline);
            free(rline);
            return -1;
        }
        if (strcmp(rline, "\n") != 0)
        {
            break;
        }
    }

    // Remove newlines
    strip_newline(lline);
    strip_newline(rline);

    // Create the first string without the padding
    char line[lcap + rcap];
    strncpy(line, "CPAIR(", lcap + rcap);
    for (size_t i = 0; i < points_len; i++)
    {
        sprintf(line + strlen(line), "{%.1f, %.1f}",
                points[i].x, points[i].y);
        if (i + 1 != points_len)
        {
            strcat(line, ", ");
        }
    }
    strncat(line, ")", 2);
    size_t len = strlen(line);

    // Calculate the padding
    size_t llen = strlen(lline);
    size_t rlen = strlen(rline);
    int padding = (llen + rlen + 2) / 2 - (len / 2);

    // Print the first line with padding
    putchar('\n');
    print_padding(padding);
    printf("%s", line);
    print_padding((rlen + llen + 2) - padding - len);
    putchar('\n');

    // Print the branch line
    print_branchline(llen, rlen);

    // Merge the trees of both children
    // The alive variable indicate if we currently have a line from that child
    // that needs to be printed. We need two of those as the trees from the
    // children can have different sized dephts.
    bool lalive = true;
    bool ralive = true;
    do
    {
        // Print the last two lines
        if (lalive)
        {
            strip_newline(lline);
            printf("%s", lline);
            print_padding(2);
        }
        else
        {
            print_padding(llen + 2);
        }
        if (ralive)
        {
            strip_newline(rline);
            printf("%s", rline);
        }
        else
        {
            print_padding(rlen);
        }
        putchar('\n');

        // Read the next line from each child
        if (getline(&lline, &lcap, lf) == -1)
        {
            lalive = false;
        }

        if (getline(&rline, &rcap, rf) == -1)
        {
            ralive = false;
        }

    } while (lalive || ralive);
    free(rline);
    free(lline);

    return 0;
}

/**
 * Wait for children to die.
 * @brief Wait for children to terminate or an attempt to not use kill as it is
 * forbidden by the coding guidelines.
 * @details This function does not check if the children terminate with an error
 * and therefore should only be used in error-handling when this (parent)
 * process knows it will terminate with an error independed from what the
 * children received.
 * This function will hang indefinitely if the children keep running. So it is 
 * the resposibility of the caller to make sure that the children will
 * terminate (for example by closing the pipes).
 * @param n The number of children we are waiting for.
 */
void lazy_kill(int n)
{
    int tmp;
    for (int i = 0; i < n; i++)
    {
        while (wait(&tmp) == -1)
            ;
    }
}

/**
 * The entrypoint of cpair. 
 * @brief The main entrypoint of this programm.
 * @details This is the first and last function to be executed.
 * @param argc The argument counter.
 * @param arv The argument vector.
 * @return Upon success EXIT_SUCCESS is returned, otherwise EXIT_FAILURE.
 */
int main(int argc, char const *argv[])
{
    // Handle arguments
    procname = argv[0];
    if (argc != 1)
    {
        fprintf(stderr, "[%s] ERROR: %s does not accept any arguments.\n",
                procname, procname);
        exit(EXIT_FAILURE);
    }

    // Parse input
    Point *points;
    size_t points_len = 0;
    if (parse_stdin(&points, &points_len) == -1)
    {
        // No need to print an error as parse_stdin does this anyway.
        exit(EXIT_FAILURE);
    }

    switch (points_len)
    {
    case (0):
        fprintf(stderr, "[%s] ERROR: No points provided via stdin!\n",
                procname);
        free(points);
        exit(EXIT_FAILURE);
        break;
    case (1):
        print_tree(points, points_len, NULL, NULL);
        free(points);
        exit(EXIT_SUCCESS);
        break;
    case (2):
        write_point(stdout, &points[0]);
        write_point(stdout, &points[1]);
        print_tree(points, points_len, NULL, NULL);
        free(points);
        exit(EXIT_SUCCESS);
        break;
    default:
        break;
    }

    // Create pipes
    int left_write_pipe[2];
    int left_read_pipe[2];
    int right_write_pipe[2];
    int right_read_pipe[2];
    if (pipe(left_write_pipe) == -1 || pipe(left_read_pipe) == -1 ||
        pipe(right_write_pipe) == -1 || pipe(right_read_pipe) == -1)
    {
        fprintf(stderr, "[%s] ERROR: Cannot pipe: %s\n", procname,
                strerror(errno));
        free(points);
        exit(EXIT_FAILURE);
    }

    // Fork and create the first child
    int child1 = fork();
    if (child1 == -1)
    {
        fprintf(stderr, "[%s] ERROR: Cannot fork: %s\n", procname,
                strerror(errno));
        free(points);
        close(left_read_pipe[0]);
        close(left_read_pipe[1]);
        close(left_write_pipe[0]);
        close(left_write_pipe[1]);
        close(right_read_pipe[0]);
        close(right_write_pipe[1]);
        close(right_read_pipe[1]);
        close(right_write_pipe[0]);
        exit(EXIT_FAILURE);
    }
    if (child1 == 0)
    {
        // Remap the pipes to stdin & stdout
        if (dup2(left_read_pipe[1], STDOUT_FILENO) == -1)
        {
            fprintf(stderr, "[%s] ERROR: Cannot dup2: %s\n",
                    procname, strerror(errno));
            free(points);
            exit(EXIT_FAILURE);
        }
        if (dup2(left_write_pipe[0], STDIN_FILENO) == -1)
        {
            fprintf(stderr, "[%s] ERROR: Cannot dup2: %s\n",
                    procname, strerror(errno));
            free(points);
            exit(EXIT_FAILURE);
        }

        // Close all pipes meant for the other child
        close(right_read_pipe[0]);
        close(right_read_pipe[1]);
        close(right_write_pipe[0]);
        close(right_write_pipe[1]);

        // Close unused pipe ends
        close(left_read_pipe[0]);
        close(left_write_pipe[1]);

        // Close the dup2 cloned pipes
        close(left_read_pipe[1]);
        close(left_write_pipe[0]);

        // Execute this program from the start
        execlp(procname, procname, NULL);
        fprintf(stderr, "[%s] ERROR: Cannot exec: %s\n", procname, strerror(errno));
        free(points);
        exit(EXIT_FAILURE);
    }

    // Create the second child
    int child2 = fork();
    if (child2 == -1)
    {
        fprintf(stderr, "[%s] ERROR: Cannot fork: %s\n", procname,
                strerror(errno));
        free(points);
        close(left_read_pipe[0]);
        close(left_read_pipe[1]);
        close(left_write_pipe[0]);
        close(left_write_pipe[1]);
        close(right_read_pipe[0]);
        close(right_write_pipe[1]);
        close(right_read_pipe[1]);
        close(right_write_pipe[0]);
        lazy_kill(1);
        exit(EXIT_FAILURE);
    }
    if (child2 == 0)
    {
        // Remap the pipes to stdin & stdout
        if (dup2(right_read_pipe[1], STDOUT_FILENO) == -1)
        {
            fprintf(stderr, "[%s] ERROR: Cannot dup2: %s\n",
                    procname, strerror(errno));
            free(points);
            exit(EXIT_FAILURE);
        }
        if (dup2(right_write_pipe[0], STDIN_FILENO) == -1)
        {
            fprintf(stderr, "[%s] ERROR: Cannot dup2: %s\n",
                    procname, strerror(errno));
            free(points);
            exit(EXIT_FAILURE);
        }
        // Close all pipes meant for the other child
        close(left_read_pipe[0]);
        close(left_read_pipe[1]);
        close(left_write_pipe[0]);
        close(left_write_pipe[1]);

        // Close unused pipe ends
        close(right_read_pipe[0]);
        close(right_write_pipe[1]);

        // Close the dup2 duplicated pipes
        close(right_read_pipe[1]);
        close(right_write_pipe[0]);

        // Execute this program from the start
        execlp(procname, procname, NULL);
        fprintf(stderr, "[%s] ERROR: Cannot exec: %s\n", procname,
                strerror(errno));
        free(points);
        lazy_kill(1);
        exit(EXIT_FAILURE);
    }

    // Close unused pipe ends in the parent and convert the remaining pipe ends
    // to stdio FILE structs.
    close(left_read_pipe[1]);
    close(left_write_pipe[0]);
    close(right_read_pipe[1]);
    close(right_write_pipe[0]);

    // Create stream file structs to use with stdio
    FILE *left_read_fd = fdopen(left_read_pipe[0], "r");
    FILE *left_write_fd = fdopen(left_write_pipe[1], "w");
    FILE *right_read_fd = fdopen(right_read_pipe[0], "r");
    FILE *right_write_fd = fdopen(right_write_pipe[1], "w");
    if (left_read_fd == NULL || left_write_fd == NULL ||
        right_read_fd == NULL || right_write_fd == NULL)
    {
        fprintf(stderr, "[%s] ERROR: Cannot create file descriptor: %s\n",
                procname, strerror(errno));
        free(points);
        close(left_read_pipe[0]);
        close(right_read_pipe[0]);
        close(left_write_pipe[1]);
        close(right_write_pipe[1]);
        lazy_kill(2);
        exit(EXIT_FAILURE);
    }

    // Calculate the arithmetic mean
    float mean = arithmetic_mean(points, points_len);

    // Send the left child the input
    for (size_t i = 0; i < points_len; i++)
    {
        // Only send the first child points that are <= mean
        if (points[i].x > mean)
        {
            continue;
        }

        if (write_point(left_write_fd, &points[i]) < 0)
        {
            fprintf(stderr,
                    "[%s] ERROR: Unable to write to the left child: %s\n",
                    procname, strerror(errno));
            free(points);
            fclose(left_read_fd);
            fclose(right_read_fd);
            fclose(left_write_fd);
            fclose(right_write_fd);
            lazy_kill(2);
            exit(EXIT_FAILURE);
        }
    }

    // Send the right child the input
    for (size_t i = 0; i < points_len; i++)
    {
        // Only send the first child points that are > mean
        if (points[i].x <= mean)
        {
            continue;
        }

        if (write_point(right_write_fd, &points[i]) < 0)
        {
            fprintf(stderr,
                    "[%s] ERROR: Unable to write to the right child: %s\n",
                    procname, strerror(errno));
            free(points);
            fclose(left_read_fd);
            fclose(right_read_fd);
            fclose(left_write_fd);
            fclose(right_write_fd);
            lazy_kill(2);
            exit(EXIT_FAILURE);
        }
    }
    fflush(left_write_fd);
    fflush(right_write_fd);
    fclose(left_write_fd);
    fclose(right_write_fd);

    // Wait for the children to die
    int status;
    int error_cnt = 0;
    while (wait(&status) == -1)
        ;
    if (WEXITSTATUS(status) != EXIT_SUCCESS)
    {
        error_cnt++;
    }
    while (wait(&status) == -1)
        ;
    if (WEXITSTATUS(status) != EXIT_SUCCESS)
    {
        error_cnt++;
    }

    // Check if the children died with an error
    if (error_cnt == 1)
    {
        fprintf(stderr,
                "[%s] One child terminated with an error\n", procname);
        free(points);
        fclose(left_read_fd);
        fclose(right_read_fd);
        exit(EXIT_FAILURE);
    }
    else if (error_cnt == 2)
    {
        fprintf(stderr,
                "[%s] Both children terminated with an error\n", procname);
        free(points);
        fclose(left_read_fd);
        fclose(right_read_fd);
        exit(EXIT_FAILURE);
    }

    // Parse the output from the children
    Point child_result[2];
    if (parse_children(left_read_fd, right_read_fd, child_result) == -1)
    {
        fprintf(stderr, "[%s] Both children terminated with zero output\n",
                procname);
        free(points);
        fclose(right_read_fd);
        fclose(left_read_fd);
        exit(EXIT_FAILURE);
    }

    Point *result[2];
    result[0] = &child_result[0];
    result[1] = &child_result[1];

    // Merge the points and find the closest pair
    merge(points, points_len, mean, &result[0], &result[1]);

    // Print the result to stdout
    write_point(stdout, result[0]);
    write_point(stdout, result[1]);
    if (print_tree(points, points_len, left_read_fd, right_read_fd) == -1)
    {
        free(points);
        fclose(right_read_fd);
        fclose(left_read_fd);
        exit(EXIT_FAILURE);
    }

    // Free all resources and exit
    free(points);
    fclose(right_read_fd);
    fclose(left_read_fd);
    return EXIT_SUCCESS;
}
