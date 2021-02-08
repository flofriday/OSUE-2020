/**
 * @file forkFFT.c
 * @author Jonny X <e12345678@student.tuwien.ac.at>
 * @date 19.12.2020
 * version: 2.0.3
 *
 * @brief This programm calculcates the FFT recursively by calling itself. No arguments should
 * be provided as the program reads from stdin after the execution. You can of course als pipe a file 
 * as input. 
 * After the caclucation the program outputs all solutions and after the solutions a tree visuaizing the
 * call graph of the children.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <complex.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define PI 3.141592654

static char *prog_name;

/**
 * @brief
 * Prints the usage format to explain which arguments are expected. 
 * After that the program ist terminated with an error code.
 * 
 * @details
 * Gets called when an input error by the user is detected.
**/
void usage(void)
{
    fprintf(stderr, "[%s] Usage: %s\n", prog_name, prog_name);
    fprintf(stderr, "Example inputs after program start: 1 0 and 1 0\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief
 * Checks if the the last character is a new line symbol (\n) and removes it.
 * 
 * @details
 * Modifies the provided input to ensure that it doesn't end with a newline symbol.
 * 
 * @param input The char* to the string which should be modified.
**/
void remove_new_line(char *input)
{
    if (input[strlen(input) - 1] == '\n')
    {
        input[strlen(input) - 1] = '\0';
    }
}

/**
 * @brief
 * Outputs the padding for the tree.
 * 
 * @details
 * Outputs the padding (left side and right side) for the tree. Should be called
 * before and after eacht child writes its tree node. Prints to stdout.
 * 
 * @param padding The amount of spaces which should be outputted.
**/
void output_padding(size_t padding)
{
    for (int i = 0; i < padding; i++)
    {
        fprintf(stdout, " ");
    }
}

/**
 * @brief
 * Outputs the branch line for a tree.
 * 
 * @details
 * Outputs the padding and the branches in one line. The left branch is dislayed by printing a slash (/) 
 * and the right one by printing a blackslash (\)
 * 
 * @param length The total line length.
**/
void output_branch_line(size_t length)
{
    for (int i = 0; i < length; i++)
    {
        //Output / at 1/4 of the length
        if (i == length / 4)
        {
            fprintf(stdout, "/");
        }
        else if (i == (length / 4) * 3)
        {
            fprintf(stdout, "\\");
        }
        else
        {
            fprintf(stdout, " ");
        }
    }
    fprintf(stdout, "\n");
}

/**
 * @brief
 * Outputs the whole current tree
 * 
 * @details
 * Outputs the new tree components of the current process and after that all tree 
 * components of its children (just like they were received).
 * 
 * @param numbers_to_children The input numbers which were received. They are included in the current tree component.
 * @param children_length the length/size of numbers_to_children.
 * @param read_child1 The FILE* from child 1 to read from it.
 * @param read_child2 The FILE* from child 2 to read from it.
**/
void output_tree(float complex *numbers_to_children, size_t children_length, FILE *read_child1, FILE *read_child2)
{
    //create char* line1 and line2 which are filled by the getline calls.
    char *line1 = NULL;
    size_t line1_cap = 0;
    char *line2 = NULL;
    size_t line2_cap = 0;

    if (getline(&line1, &line1_cap, read_child1) == -1)
    {
        /*
        should not happen. If it does no tree is outputted. 
        Assert is not used to not crash the program if it does.
        */
        return;
    }

    if (getline(&line2, &line2_cap, read_child2) == -1)
    {
        /*
        should not happen. If it does no tree is outputted. 
        Assert is not used to not crash the program if it does.
        */
        return;
    }

    /*
    First remove newline characters from both lines.
    Then calculate the line size by adding the first line length of child 1 and the first line length of child 2.
    Create a new char array which holds the string value of the new line (which should late be printed)
    */
    remove_new_line(line1);
    remove_new_line(line2);
    size_t array_cap = strlen(line1) + strlen(line2) + 2;
    char new_tree_line[array_cap];
    strncpy(new_tree_line, "FFT(", array_cap);

    // Add each complex number pair which was received as input to the string.
    for (int i = 0; i < children_length; i++)
    {
        sprintf(new_tree_line + strlen(new_tree_line), "{%f %f}", creal(numbers_to_children[i]), cimag(numbers_to_children[i]));
    }
    strcat(new_tree_line, ")");

    /* 
    Calculate the right and left padding and output it before and after the new tree line is printed.
    After that output the branch line to display the branches.
    */
    fprintf(stdout, "\n");
    size_t padding_left = (array_cap - strlen(new_tree_line)) / 2;
    size_t padding_right = array_cap - strlen(new_tree_line) - padding_left;
    output_padding(padding_left);
    fprintf(stdout, "%s", new_tree_line);
    output_padding(padding_right);
    fprintf(stdout, "\n");
    output_branch_line(array_cap);

    /* 
    Print out first merged line (from child 1 and child2)
    Then remove the new lines from all other lines from chld 1 and child 2. Merge them together and 
    output them exactly like they were received. Lastly free all resources.
    */
    fprintf(stdout, "%s  %s\n", line1, line2);
    while (true)
    {
        if (getline(&line1, &line1_cap, read_child1) == -1)
        {
            break;
        }

        if (getline(&line2, &line2_cap, read_child2) == -1)
        {
            break;
        }

        remove_new_line(line1);
        remove_new_line(line2);
        fprintf(stdout, "%s  %s\n", line1, line2);
    }

    //free the line allocated by getline
    free(line1);
    free(line2);
}

/**
 * @brief
 * Parses the input into a complex number
 * 
 * @details
 * Reads from the char* and parses the string to a complex number. This complex number is written to the
 * provided complex number pointer.
 * 
 * @param input The input which should be read an parsed.
 * @param complex_number The pointer where parsed result is written to.
 * @return Returns 0 if successfull, otherwise -1
**/
int parse_input(const char *input, float complex *complex_number)
{
    float real_value = 0;
    float im_value = 0;
    char *endpointer1 = NULL;
    char *endpointer2 = NULL;

    errno = 0;
    real_value = strtof(input, &endpointer1);

    // If strtof fails, 0 is returned (see man page). This is exactly the behaviour we want.
    im_value = strtof(endpointer1, &endpointer2);

    // check if strtof got an error when parsing the real value
    if (real_value == 0 && (errno != 0 || endpointer2 == input))
    {
        fprintf(stderr, "[%s] Error when parsing values (%s)\n", prog_name, strerror(errno));
        return -1;
    }

    //check if input could be parsed completely and no other values are left
    if (*endpointer2 != '\0' && *endpointer2 != '\n')
    {
        fprintf(stderr, "[%s] Error when parsing values (%s)\n", prog_name, endpointer2);
        return -1;
    }

    *complex_number = real_value + im_value * I;
    return 0;
}

/**
 * @brief
 * Wait for children to terminate
 * 
 * @details
 * Functon waits for both children to terminate and proceeds once both children have terminated.
 * @param pid1 The pid from child1.
 * @param pid2 The pid from child2.
**/
void wait_for_both_children(pid_t pid1, pid_t pid2)
{
    int status = 0;
    //wait for children to terminate
    while (waitpid(pid1, &status, 0) == -1)
        ;
    while (waitpid(pid2, &status, 0) == -1)
        ;
}

/**
 * Program entry point.
 * @brief The program recursively calls itself to calculate the FFT by using the Cooley-Tukey algorithm.
 *  
 * @details The program first reads the inputs an forks if more than one input is provided. 
 * Each child repeats this process until only one input is provided. That result is returned
 * to the parent which uses this to calculate the FFT.
 * 
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char **argv)
{
    //Exit if arguments were provided
    prog_name = argv[0];
    if (argc != 1)
    {
        usage();
    }

    //create char* line1 and line2 which are filled by the getline calls.
    char *line1 = NULL;
    size_t line1_cap = 0;
    char *line2 = NULL;
    size_t line2_cap = 0;

    // No input was provided -> Error
    if (getline(&line1, &line1_cap, stdin) == -1)
    {
        fprintf(stderr, "[%s] Error no input provided\n", prog_name);
        free(line1);
        exit(EXIT_FAILURE);
    }
    /* 
    Only one input (=complex number) was provided -> write it to stdout. This means that this process
    is an leaf node.
    */
    if (getline(&line2, &line2_cap, stdin) == -1)
    {
        // print the complex number which was received to stdout.
        float complex complex_number;
        
        if (parse_input(line1, &complex_number) == -1)
        {
            free(line1);
            free(line2);
            usage();
        }
        fprintf(stdout, "%f %f\n", creal(complex_number), cimag(complex_number));

        // print the tree leaf element.
        fprintf(stdout, "\nFFT({%f %f})\n", creal(complex_number), cimag(complex_number));

        // free resources and exit.
        free(line1);
        free(line2);
        exit(EXIT_SUCCESS);
    }

    /*
    IMPORTANT: IF THE CODE GOT HERE THE RECURSION GOES ON. THIS MEANS THAT MORE THAN ONE INPUT (=COMPLEX NUMBER)
    WAS RECEIVED.
    */

    // Create the file descriptors which are used to communicate with the childs
    int fd_write_to_child1[2];
    int fd_read_from_child1[2];
    int fd_write_to_child2[2];
    int fd_read_from_child2[2];

    // Create all four pipes and check if the creation of the pipe was successful (Return value != -1)
    if (pipe(fd_write_to_child1) == -1)
    {
        fprintf(stderr, "[%s] Error pipe(fd_write_to_child1) failed\n", prog_name);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }
    if (pipe(fd_read_from_child1) == -1)
    {
        fprintf(stderr, "[%s] Error pipe(fd_read_from_child1) failed\n", prog_name);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }
    if (pipe(fd_write_to_child2) == -1)
    {
        fprintf(stderr, "[%s] Error pipe(fd_write_to_child2) failed\n", prog_name);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }
    if (pipe(fd_read_from_child2) == -1)
    {
        fprintf(stderr, "[%s] Error pipe(fd_write_to_child1) failed\n", prog_name);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }

    // Fork the first time and check if the forking was successful
    pid_t pid1 = fork();

    switch (pid1)
    {
    case -1:
        fprintf(stderr, "Cannot fork!\n");
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
        break;
    case 0:
        // CHILD 1

        /* dupe fds:
        1) so the child can write to stdout and the parent can read from it with the pipe
        2) so the child can read from stdin and the parent can write to it with the pipe.
        */
        if (dup2(fd_write_to_child1[0], STDIN_FILENO) == -1)
        {
            fprintf(stderr, "[%s] Error: dup2(fd_write_to_child1[0]) failed (%s)\n", prog_name, strerror(errno));
        }
        if (dup2(fd_read_from_child1[1], STDOUT_FILENO) == -1)
        {
            fprintf(stderr, "[%s] Error: dup2(fd_read_from_child1[1]) failed (%s)\n", prog_name, strerror(errno));
        }

        //close all pipes from child 1 because the needed ones were already duped
        close(fd_write_to_child1[0]);
        close(fd_write_to_child1[1]);
        close(fd_read_from_child1[0]);
        close(fd_read_from_child1[1]);

        //closes all pipes from child 2 as they are not needed in child 1
        close(fd_write_to_child2[0]);
        close(fd_write_to_child2[1]);
        close(fd_read_from_child2[0]);
        close(fd_read_from_child2[1]);

        //exec the programm (forkFFT)
        execl(prog_name, prog_name, NULL);

        // program only gets here if exec failed -> Error: print error, free resources and exit with failure
        fprintf(stderr, "[%s] Error: Exec from child 1 failed: %s\n", prog_name, strerror(errno));
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
        break;
    default:
        break;
    }

    /*
    If the process got here it means that it is the parent (the child was filtered with the switch)
    Fork the second time and check if the forking was successful
    */
    pid_t pid2 = fork();

    switch (pid2)
    {
    case -1:
        fprintf(stderr, "Cannot fork!\n");
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
        break;
    case 0:
        // CHILD 2

        /* dupe fds:
        1) so the child can write to stdout and the parent can read from it with the pipe
        2) so the child can read from stdin and the parent can write to it with the pipe.
        */
        if (dup2(fd_write_to_child2[0], STDIN_FILENO) == -1)
        {
            fprintf(stderr, "[%s] Error: dup2(fd_write_to_child2[0]) failed (%s)\n", prog_name, strerror(errno));
        }
        if (dup2(fd_read_from_child2[1], STDOUT_FILENO) == -1)
        {
            fprintf(stderr, "[%s] Error: dup2(fd_read_from_child2) failed (%s)\n", prog_name, strerror(errno));
        }

        //closes all pipes from child 2 because the needed ones were already duped
        close(fd_write_to_child2[0]);
        close(fd_write_to_child2[1]);
        close(fd_read_from_child2[0]);
        close(fd_read_from_child2[1]);

        //closes all pipes from child 1 as they are not needed in child 2
        close(fd_write_to_child1[0]);
        close(fd_write_to_child1[1]);
        close(fd_read_from_child1[0]);
        close(fd_read_from_child1[1]);

        //exec the programm (forkFFT)
        execl(prog_name, prog_name, NULL);

        // program only gets here if exec failed -> Error: print error, free resources and exit with failure
        fprintf(stderr, "[%s] Error: Exec from child 1 failed (%s)\n", prog_name, strerror(errno));
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
        break;
    }

    /*
    If the process got here it means that it is the parent of both children.
    Close all unneeded ends of the pipes
    1) read end of the write pipe to child 1
    2) write end of the read pipe from child 1
    3) read end of the write pipe to child 2
    4) write end of the read pipe from child 2
    */
    close(fd_write_to_child1[0]);
    close(fd_read_from_child1[1]);
    close(fd_write_to_child2[0]);
    close(fd_read_from_child2[1]);

    // Create file pointer and open the fd so we can operate with Standard I/O lib.
    FILE *write_child1 = fdopen(fd_write_to_child1[1], "w");
    FILE *write_child2 = fdopen(fd_write_to_child2[1], "w");
    if (write_child1 == NULL)
    {
        fprintf(stderr, "[%s] Error fdopen(fd_write_to_child1): %s\n", prog_name, strerror(errno));
        free(line1);
        free(line2);
        close(fd_write_to_child1[1]);
        close(fd_read_from_child1[0]);
        close(fd_write_to_child2[0]);
        close(fd_read_from_child2[1]);

        wait_for_both_children(pid1, pid2);
        exit(EXIT_FAILURE);
    }
    if (write_child2 == NULL)
    {
        fprintf(stderr, "[%s] Error fdopen(fd_write_to_child2): %s\n", prog_name, strerror(errno));
        free(line1);
        free(line2);
        close(fd_write_to_child1[1]);
        close(fd_read_from_child1[0]);
        close(fd_write_to_child2[0]);
        close(fd_read_from_child2[1]);

        wait_for_both_children(pid1, pid2);
        exit(EXIT_FAILURE);
    }

    // Creat a complex pointer. This is later needed to generate the tree.
    size_t output_to_children_cap = 2;
    float complex *output_to_children = malloc(output_to_children_cap * sizeof(float complex));
    if (output_to_children == NULL)
    {
        fprintf(stderr, "[%s] Error malloc failed: %s\n", prog_name, strerror(errno));
        free(output_to_children);
        free(line1);
        free(line2);
        fclose(write_child1);
        fclose(write_child2);
        wait_for_both_children(pid1, pid2);
        exit(EXIT_FAILURE);
    }

    // write the first line (2 lines in total) to each child beause they were already read to check if there are
    // more than 1 input.
    if (fprintf(write_child1, "%s", line1) < 0)
    {
        fprintf(stderr, "[%s] Error fprintf(write_child1): %s\n", prog_name, strerror(errno));
        free(output_to_children);
        free(line1);
        free(line2);
        fclose(write_child1);
        fclose(write_child2);
        wait_for_both_children(pid1, pid2);
        exit(EXIT_FAILURE);
    }

    if (fprintf(write_child2, "%s", line2) < 0)
    {
        fprintf(stderr, "[%s] Error fprintf(write_child2): %s\n", prog_name, strerror(errno));
        free(output_to_children);
        free(line1);
        free(line2);
        fclose(write_child1);
        fclose(write_child2);
        wait_for_both_children(pid1, pid2);
        exit(EXIT_FAILURE);
    }

    // Safe both received inputs into the complex pointer.
    if (parse_input(line1, &output_to_children[0]) == -1)
    {
        free(output_to_children);
        free(line1);
        free(line2);
        fclose(write_child1);
        fclose(write_child2);
        wait_for_both_children(pid1, pid2);
        usage();
    }
    if (parse_input(line2, &output_to_children[1]) == -1)
    {
        free(output_to_children);
        free(line1);
        free(line2);
        fclose(write_child1);
        fclose(write_child2);
        wait_for_both_children(pid1, pid2);
        usage();
    }

    size_t n = 2;
    // write the rest of the received lines to the children and the complex pointer.
    while (true)
    {
        // Increase (double) capacity of pointer if needed
        while (n + 2 >= output_to_children_cap)
        {
            output_to_children_cap *= 2;
            float complex *output_to_children_temp = realloc(output_to_children, output_to_children_cap * sizeof(float complex));
            if (output_to_children_temp == NULL)
            {
                fprintf(stderr, "[%s] Error realloc failed\n", prog_name);
                free(output_to_children);
                free(line1);
                free(line2);
                fclose(write_child1);
                fclose(write_child2);
                wait_for_both_children(pid1, pid2);
                exit(EXIT_FAILURE);
            }
            output_to_children = output_to_children_temp;
        }

        // Read next line of child 1, parse it and safe it to the complex pointer. Lastly write the unparsed line
        // to child 1
        if (getline(&line1, &line1_cap, stdin) == -1)
        {
            break;
        }
        if (parse_input(line1, &output_to_children[n]) == -1)
        {
            free(output_to_children);
            free(line1);
            free(line2);
            fclose(write_child1);
            fclose(write_child2);
            wait_for_both_children(pid1, pid2);
            usage();
        }
        n++;
        fprintf(write_child1, "%s", line1);

        // Read next line of child 2, parse it and safe it to the complex pointer. Lastly write the unparsed line
        // to child 2
        if (getline(&line2, &line2_cap, stdin) == -1)
        {
            break;
        }

        if (parse_input(line2, &output_to_children[n]) == -1)
        {
            free(output_to_children);
            free(line1);
            free(line2);
            fclose(write_child1);
            fclose(write_child2);
            wait_for_both_children(pid1, pid2);
            usage();
        }
        n++;
        fprintf(write_child2, "%s", line2);
        
    }


    fflush(write_child1);
    fflush(write_child2);

    // The input must be even in every child. If it is not: free resources, wait for the children to terminate
    // and exit.
    if (n % 2 != 0)
    {
        fprintf(stderr, "[%s] Error input is not even. (in root or any child process)\n", prog_name);

        //close pipes to ensure that children die
        fclose(write_child1);
        fclose(write_child2);
        free(line1);
        free(line2);
        wait_for_both_children(pid1, pid2);
        exit(EXIT_FAILURE);
    }

    // all inputs was read: close File pointers.
    fclose(write_child1);
    fclose(write_child2);

    // Wait for children to do their work and terminate.
    int status1;
    int status2;
    while (waitpid(pid1, &status1, 0) == -1)
    {
    }
    while (waitpid(pid2, &status2, 0) == -1)
    {
    }

    // Check if both children did exit with success.
    if (WEXITSTATUS(status1) != EXIT_SUCCESS)
    {
        fprintf(stderr, "[%s] Error child1 exited with error\n", prog_name);
        close(fd_read_from_child1[0]);
        close(fd_read_from_child2[0]);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }
    if (WEXITSTATUS(status2) != EXIT_SUCCESS)
    {
        fprintf(stderr, "[%s] Error child2 exited with error\n", prog_name);
        close(fd_read_from_child1[0]);
        close(fd_read_from_child2[0]);
        free(line1);
        free(line2);
        exit(EXIT_FAILURE);
    }

    // If the process got here this means that the children all finished successfully and we can now read
    // the results from them.
    // Create file pointer and open the fd so we can operate with Standard I/O lib.
    FILE *read_child1 = fdopen(fd_read_from_child1[0], "r");
    if (read_child1 == NULL)
    {
        fprintf(stderr, "[%s] Error fdopen(fd_read_from_child1): %s\n", prog_name, strerror(errno));
        free(line1);
        free(line2);
        close(fd_read_from_child1[0]);
        close(fd_read_from_child2[0]);
        exit(EXIT_FAILURE);
    }
    FILE *read_child2 = fdopen(fd_read_from_child2[0], "r");
    if (read_child2 == NULL)
    {
        fprintf(stderr, "[%s] Error fdopen(fd_read_from_child2): %s\n", prog_name, strerror(errno));
        free(line1);
        free(line2);
        close(fd_read_from_child1[0]);
        fclose(read_child1);
        exit(EXIT_FAILURE);
    }

    int k = 0;
    // Create an array which holfs the second half of the values. (Is outputted after all first half
    // values were printed)
    float complex second_half[n / 2];

    // Read each line from child 1 and 2. Calculate the result1 and result2 with both values. Output result1 and
    // store result2 in the second_half array.
    while (true)
    {
        bool exit_flag = false;
        float complex re;
        float complex ro;

        // Read from child 1
        if (getline(&line1, &line1_cap, read_child1) == -1 || strcmp(line1, "\n") == 0)
        {
            exit_flag = true;
        }
        // Read from child 2
        if (getline(&line2, &line2_cap, read_child2) == -1 || strcmp(line2, "\n") == 0)
        {
            exit_flag = true;
        }

        // If the last line was read: exit the loop.
        if (exit_flag)
        {
            break;
        }

        //parse both line and store them in 2 complex numbers each. If it fails free resources and exit
        if (parse_input(line1, &re) == -1)
        {
            fclose(read_child1);
            fclose(read_child2);
            free(output_to_children);
            free(line1);
            free(line2);
            usage();
        }
        if (parse_input(line2, &ro) == -1)
        {
            fclose(read_child1);
            fclose(read_child2);
            free(output_to_children);
            free(line1);
            free(line2);
            usage();
        }

        //calcualte result from children
        float complex result1 = re + (cos((-(2 * PI) / n) * k) + I * sin((-(2 * PI) / n) * k)) * ro;
        float complex result2 = re - (cos((-(2 * PI) / n) * k) + I * sin((-(2 * PI) / n) * k)) * ro;

        second_half[k] = result2;

        //write the result R[k] to stdout
        fprintf(stdout, "%f %f\n", creal(result1), cimag(result1));
        k += 1;
    }

    // Ouput all second half values. R[k+n/2]
    for (int i = 0; i < n / 2; i++)
    {
        fprintf(stdout, "%f %f\n", creal(second_half[i]), cimag(second_half[i]));
        
    }

    // Ouput the whole tree
    output_tree(output_to_children, n, read_child1, read_child2);
    fflush(stdout);

    // Close all remaining resources.
    fclose(read_child1);
    fclose(read_child2);

    free(output_to_children);
    free(line1);
    free(line2);

    exit(EXIT_SUCCESS);
}