/**
 * @project: Integer Multiplication
 * @module intmul
 * @author Johannes Zottele 11911133
 * @version 1.0
 * @date 17.12.2020
 * @section File Overview
 * Treerep includes all functions to manage the treerepresentation of the child processes of a process
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "treerep.h"

#define EXIT_ERR(msg, iserrno)                                                                           \
    {                                                                                                    \
        if (iserrno == 1)                                                                                \
            (void)fprintf(stderr, "[%s:%d] ERROR: " msg " : %s\n", __FILE__, __LINE__, strerror(errno)); \
        else                                                                                             \
            (void)fprintf(stderr, "[%s:%d] ERROR: " msg "\n", __FILE__, __LINE__);                       \
        exit(EXIT_FAILURE);                                                                              \
    }

void process_to_string(char **str, char *nr1, char *nr2)
{

    char *intm = "intmul(";
    size_t baselen = strlen(intm);
    size_t len1 = strlen(nr1) + 1;
    size_t len2 = strlen(nr2) + 1;
    size_t addlen = len1 + len2;
    size_t tlen = baselen + addlen;
    *str = malloc(tlen + 1);
    strcpy(*str, intm);

    strcpy(*str + baselen, nr1);
    (*str)[baselen + len1 - 1] = ',';
    strcpy(*str + baselen + len1, nr2);
    (*str)[tlen - 1] = ')';
    (*str)[tlen] = '\0';
}

/**
 * @brief Adds count many space characters to string (before or after (bfr_aftr 0 or 1))
 */
static int add_spaces(char **str, int count, int bfr_aftr)
{
    int strl = strlen(*str);
    int newLen = strl + count;

    *str = realloc(*str, newLen + 1);

    if (bfr_aftr)
    {
        memset(*str + strl, ' ', count);
    }
    else
    {
        strcpy(*str + count, *str);
        memset(*str, ' ', count);
    }
    (*str)[newLen] = '\0';
    return newLen;
}

void read_and_print(int pipefd[4][2], char *pname)
{

    FILE *pipe1 = fdopen(pipefd[0][0], "r");
    FILE *pipe2 = fdopen(pipefd[1][0], "r");
    FILE *pipe3 = fdopen(pipefd[2][0], "r");
    FILE *pipe4 = fdopen(pipefd[3][0], "r");

    if (pipe1 == NULL || pipe2 == NULL || pipe3 == NULL || pipe4 == NULL)
        EXIT_ERR("Could not open pipe", 1);

    char *line1 = NULL;
    size_t lc1 = 0;
    ssize_t len1;

    char *line2 = NULL;
    size_t lc2 = 0;
    ssize_t len2;

    char *line3 = NULL;
    size_t lc3 = 0;
    ssize_t len3;

    char *line4 = NULL;
    size_t lc4 = 0;
    ssize_t len4;

    int first = 1;

    size_t spacebtw = 0;
    size_t blockwidth = 0;
    size_t fnamelen = strlen(pname);

    while ((len1 = getline(&line1, &lc1, pipe1)) != -1 && (len2 = getline(&line2, &lc2, pipe2)) != -1 && (len3 = getline(&line3, &lc3, pipe3)) != -1 && (len4 = getline(&line4, &lc4, pipe4)) != -1)
    {
        len1 -= 1;
        len2 -= 1;
        len3 -= 1;
        len4 -= 1;

        line1[len1] = '\0';
        line2[len2] = '\0';
        line3[len3] = '\0';
        line4[len4] = '\0';

        if (first)
        {

            first = 0;
            spacebtw = 1;
            blockwidth = len1 + spacebtw + len2 + spacebtw + len3 + spacebtw + len4;
            int tmargin = blockwidth - fnamelen;

            if (blockwidth < fnamelen)
            {
                blockwidth = fnamelen;
                spacebtw = blockwidth - len1 - len2 - len3 - len4;
                tmargin = 0;
            }

            int lp = 0;
            lp = add_spaces(&pname, tmargin / 2, 0);
            lp = add_spaces(&pname, tmargin / 2, 1);

            int correction_name = blockwidth - lp > 0 ? blockwidth - lp : 0;
            add_spaces(&pname, correction_name, 1);

            fprintf(stdout, "%s\n", pname);
            free(pname);

            char *path1 = malloc(2);
            int l1 = 0;
            path1[0] = '/';
            path1[1] = '\0';
            l1 = add_spaces(&path1, len1 / 2, 0);
            l1 = add_spaces(&path1, len1 / 2 - 1, 1);

            char *path2 = malloc(2);
            int l2 = 0;
            path2[0] = '/';
            path2[1] = '\0';
            l2 = add_spaces(&path2, len2 / 2 + spacebtw, 0);
            l2 = add_spaces(&path2, len2 / 2 - 1, 1);

            char *path3 = malloc(2);
            int l3 = 0;
            path3[0] = '\\';
            path3[1] = '\0';
            l3 = add_spaces(&path3, len3 / 2 + spacebtw, 0);
            l3 = add_spaces(&path3, len3 / 2 - 1, 1);

            char *path4 = malloc(2);
            int l4 = 0;
            path4[0] = '\\';
            path4[1] = '\0';
            l4 = add_spaces(&path4, len4 / 2 + spacebtw, 0);
            l4 = add_spaces(&path4, len4 / 2 - 1, 1);

            int sumlen = l1 + l2 + l3 + l4;
            int correction_paths = blockwidth - sumlen > 0 ? blockwidth - sumlen : 0;
            add_spaces(&path4, correction_paths, 1);

            fprintf(stdout, "%s%s%s%s\n", path1, path2, path3, path4);

            free(path1);
            free(path2);
            free(path3);
            free(path4);
        }

        add_spaces(&line1, spacebtw, 1);
        add_spaces(&line2, spacebtw, 1);
        add_spaces(&line3, spacebtw, 1);
        fprintf(stdout, "%s%s%s%s\n", line1, line2, line3, line4);
    }

    free(line1);
    free(line2);
    free(line3);
    free(line4);
}