/**
 * @project: Integer Multiplication
 * @module intmul
 * @author Johannes Zottele 11911133
 * @version 1.0
 * @date 15.12.2020
 * @section File Overview
 * intmul is responsible for the procedure of forking and piping between all processes.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "hexcalc.h"
#include "treerep.h"

#define EXIT_ERR(msg, iserrno)                                                                           \
    {                                                                                                    \
        if (iserrno == 1)                                                                                \
            (void)fprintf(stderr, "[%s:%d] ERROR: " msg " : %s\n", __FILE__, __LINE__, strerror(errno)); \
        else                                                                                             \
            (void)fprintf(stderr, "[%s:%d] ERROR: " msg "\n", __FILE__, __LINE__);                       \
        wait_handler(1);                                                                                 \
    }

/**
 * @brief File descriptors for the pipe from an child in direction to the parent.
 */
int pipeout[4][2];

int treerep;

/**
 * @brief Handles the waiting for all child processes.
 * 
 * @details If EXIT_ERR() is called, the wait_handler() will be called with err=1. So after receiving all child processes, this process
 * exits with EXIT_FAILURE without continueing the program.
 */
static void wait_handler(int err)
{
    int status,
        pid, error = err;
    while ((pid = wait(&status)) != -1)
    {
        if (WEXITSTATUS(status) != EXIT_SUCCESS)
            error = 1;
    }

    if (error)
        exit(EXIT_FAILURE);

    if (errno != ECHILD)
        EXIT_ERR("cannot wait!", 1)
}

/**
 * @brief Looks for two string from stdin. 
 * 
 * @details Calls EXIT_ERR() if input is not valid. Stores strings in given pointerpointer.
 * 
 * @return returns the length of one string.
 */
static size_t get_values(char **a, char **b)
{
    size_t hexlen = 0;

    char *line;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, stdin)) != -1)
    {

        if (linelen == 1)
            EXIT_ERR("Length of number was 0", 0)
        if (hexlen != 0 && linelen - 1 != hexlen)
            EXIT_ERR("Numbers of different lengths", 0)

        if (hexlen == 0)
        {
            *a = malloc(linelen);
            strcpy(*a, line);
            hexlen = linelen - 1;
            if (hexlen % 2 != 0 && hexlen != 1)
                EXIT_ERR("number is not even", 0);
        }
        else
        {
            *b = malloc(linelen);
            strcpy(*b, line);
            break;
        }
    }

    return hexlen;
}

/**
 * @brief Splits given string line into two and stores them in the pointerpointer. Uses malloc so its important to free them.
 * Each string is ending with '\n'.
 */
static void gen_half_strlines(size_t len, char *A, char **Ah, char **Al)
{
    int lh = len / 2;
    int ll = len - lh;

    *Ah = malloc(lh + 2);
    *Al = malloc(ll + 2);

    memcpy(*Ah, A, lh);
    memcpy(*Al, A + lh, ll);
    Ah[0][lh] = '\n';
    Ah[0][lh + 1] = '\0';
    Al[0][ll] = '\n';
    Al[0][ll + 1] = '\0';
}

/**
 * @brief Writes data to pipe (given through the file descriptor)
 */
static void write_to_pipe(int fd, char *data)
{
    FILE *f = fdopen(fd, "w");
    if (fputs(data, f) == -1)
        EXIT_ERR("cannot write to pipe", 1);

    fflush(f);
}

/**
 * @brief Reads from all outpipe reading ends and stores the result in given result pointer array.
 */
static void read_from_pipes(char *results[4])
{

    for (int i = 0; i < 4; i++)
    {
        FILE *out = fdopen(pipeout[i][0], "r");
        size_t lencap = 0;
        ssize_t len = 0;
        if ((len = getline(&results[i], &lencap, out)) == -1)
            EXIT_ERR("failed to read result", 1)

        results[i][len - 1] = '\0';
    }
}

/**
 * @brief Creates 4 childprocesses with in and out pipe. 
 * It also redirects the readend of the inpipe to the stdin and the writeend of the outpipe to the stdout.
 * After forking and piping the parent process writes the data to the child processes.
 */
static void fork_and_pipe(int hexlen, char *A, char *B)
{
    char *Ah;
    char *Al;
    char *Bh;
    char *Bl;

    gen_half_strlines(hexlen, A, &Ah, &Al);
    gen_half_strlines(hexlen, B, &Bh, &Bl);

    free(A);
    free(B);

    // Ah * Bh = cid1
    // Ah * Bl = cid2
    // Al * Bh = cid3
    // Al * Bl = cid4
    pid_t cdi[4];
    int pipein[4][2];

    for (int i = 0; i < 4; i++)
    {
        pipe(pipein[i]);
        pipe(pipeout[i]);

        cdi[i] = fork();

        switch (cdi[i])
        {
        case -1:
            EXIT_ERR("Cannot fork!", 1);
            break;
        case 0:
            close(pipein[i][1]);
            close(pipeout[i][0]);

            if (dup2(pipein[i][0], STDIN_FILENO) == -1 || dup2(pipeout[i][1], STDOUT_FILENO) == -1)
                EXIT_ERR("could not redirect pipe", 1);

            close(pipein[i][0]);
            close(pipeout[i][1]);

            if (treerep)
                execlp("./intmul", "./intmul", "-t", NULL);
            else
                execlp("./intmul", "./intmul", NULL);

            EXIT_ERR("Could not execute", 1);
            break;
        default:
            close(pipein[i][0]);
            close(pipeout[i][1]);

            int pfd = pipein[i][1];

            switch (i)
            {
            case 0:
                write_to_pipe(pfd, Ah);
                write_to_pipe(pfd, Bh);
                break;
            case 1:
                write_to_pipe(pfd, Ah);
                write_to_pipe(pfd, Bl);
                break;
            case 2:
                write_to_pipe(pfd, Al);
                write_to_pipe(pfd, Bh);
                break;
            default:
                write_to_pipe(pfd, Al);
                write_to_pipe(pfd, Bl);
                break;
            };
            break;
        }
    }

    free(Ah);
    free(Al);
    free(Bh);
    free(Bl);
}

/**
 * @brief Reads 2 hex strings from stdin, if the length of them is 1, they are going to be added and printed to stdout as hex string.
 * Otherwise fork_and_pipe() is called. After all child processes are finished, the process reads from the pipeout (result of the child processes).
 * Then it calls calcQuadResult() from hexcalc.c, where the caculation for the result is managed.
 * Last but not least, the result is printed to stdout.
 * 
 * If the flag "-t" is set, an treerepresentation of all child processes is printed instead of the result. It ueses process_to_string() and read_and_print() from treerep.c 
 */
int main(int argc, char *argv[])
{
    treerep = 0;
    if (argc > 1 && strcmp(argv[1], "-t") == 0)
        treerep = 1;

    char *A;
    char *B;

    size_t hexlen = get_values(&A, &B);

    A[hexlen] = '\0';
    B[hexlen] = '\0';

    char *pname;
    if (treerep)
        process_to_string(&pname, A, B);

    if (hexlen == 1)
    {
        if (treerep)
            fprintf(stdout, "%s\n", pname);
        else
        {
            unsigned int a = strtol(A, NULL, 16);
            unsigned int b = strtol(B, NULL, 16);
            fprintf(stdout, "%X\n", (a * b));
            fflush(stdout);
        }
        exit(EXIT_SUCCESS);
    }

    fork_and_pipe(hexlen, A, B);

    wait_handler(0);

    if (treerep)
        read_and_print(pipeout, pname);
    else
    {
        char *results[4];
        read_from_pipes(results);

        calcQuadResult(results, results[1], results[2], results[3], hexlen);

        fprintf(stdout, "%s\n", results[0]);
        fflush(stdout);

        free(results[0]);
    }

    exit(EXIT_SUCCESS);
    return 0;
}
