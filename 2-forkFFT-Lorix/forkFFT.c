/**
 * @file forkFFT.c
 * @author xxxxx@tuwien.ac.at>
 * @date 17.12.2020
 * 
 * @brief Fast Fourrier Transformation using forks.
 * 
 * This program computes the fast fourrier transformation using forks.
 */

#include "forkFFT.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <string.h> 
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

static char * program = "<not yet encountered>";

/**
 * @brief This function writes helpful messages to stderr.
 * @details global variables: program
 */
void usage(char * message) {
    fprintf(stderr, "USAGE: %s \n", program);
    exit(EXIT_FAILURE);
}

/**
 * @brief This function writes helpful messages to stderr.
 * @details global variables: program
 */
void error_exit(char * message) {
    if(errno == 0){
        fprintf(stderr, "%s: %s\n", message, program);
    } else {
        fprintf(stderr, "%s: %s %s\n", message, strerror(errno), program);
    }
    
    exit(EXIT_FAILURE);
}
/**
 * @brief This function spawns a child and creates pipes.
 * @param info Information about the read, write end and the pid of the child that is spawned is saved in here.
 */
static void spawn_child(info_t * info) {
    // setting up common variables.
    int pipes[2][2];
    if((pipe(pipes[0]) == -1) || (pipe(pipes[1]) == -1)){
        error_exit("Failed to pipe!");
    }
    info -> pid = fork();
    switch(info -> pid) {
        case -1:
            error_exit("Failed to fork!");
            break;
        case 0:
            close(pipes[0][1]);
            close(pipes[1][0]);
            if(((dup2(pipes[0][0],STDIN_FILENO)) ==-1) || ((dup2(pipes[1][1],STDOUT_FILENO)) ==-1)){
                error_exit("Failed to redirect stdin or stdout!");
            }
            close(pipes[0][0]);
            close(pipes[1][1]);
            if(execlp(program,program,NULL) == -1){
                error_exit("Failed to execute program!");
            }
            assert(0);
        default:
            close(pipes[0][0]);
            close(pipes[1][1]);
            info -> write = pipes[0][1];
            info -> read = pipes[1][0];
            break;
    }
}
//FINISHED
/**
 * @brief turns a string into a complex_t.
 * @details throws an error if the string can't be converted into a complex_t.
 * @param buf string that is turned into complex_t.
 * @param num complex_t in which the parsed string is stored.
 */
static void string_to_imaginary(char * buf, complex_t * num){
    char * endptr;
    errno = 0;
    num -> real = strtof(buf, &endptr);
    if(errno == ERANGE || (errno != 0 && num -> real == 0)) {
        error_exit("Input is out of bounds!");
    }
    if(endptr == buf) {
        error_exit("Input is empty!");
    }
    if(*endptr == ' ') {
        num -> imaginary = strtof(endptr, &endptr);
        if(errno == ERANGE || (errno != 0 && num -> real == 0)){
            error_exit("Input imaginary number is out of bounds!");
        }
        if(endptr == buf) {
            error_exit("Input imaginary number is empty!");
        }
        if((strcmp("*i\n", endptr)) != 0) {
            error_exit("Input does not end correctl [*i\\n]");
        }
    } else if(* endptr == '\n') {
        num -> imaginary = 0;
    } else {
        error_exit("Input has to end end with \\n and must be a valid number");
    }
}

/**
 * @brief completes the butterfly operation for two values and returns the value in them.
 * @param e Re[k] after the function R[k].
 * @param o Ro[k] after the function R[k+n/2].
 * @param k current position in Re and Ro.
 * @param n expected size of R.
 */
static void butterfly(complex_t * e, complex_t * o, int k, int n){
    complex_t c = {.real=cos((-((2*PI)/n)) * k),.imaginary=sin((-((2*PI)/n)) * k)},x;;
    x.real = c.real;
    x.imaginary = c.imaginary;
    
    c.real = x.real * o->real - x.imaginary * o->imaginary;
    c.imaginary = x.real * o->imaginary + x.imaginary * o->real;  

    o->real = e->real - c.real;
    o->imaginary = e->imaginary - c.imaginary;

    e->real = e->real + c.real;
    e->imaginary = e->imaginary + c.imaginary;
}
/**
 * @brief wrapper for fgets to deal with EINTR.
 * @param buffer buffer to read into.
 * @param in File to read from.
 * @return returns -1 if something went wrong else 0.
*/
static int read_data(char * buffer, FILE * in){
    while (fgets(buffer, MAX_LINE_LENGTH, in) == NULL){
        if(errno == EINTR)continue;
        return -1;
    }
    return 0;
}

/**
 * @brief wrapper for fputs to deal with EINTR.
 * @param buffer buffer to write to out.
 * @param ot File to write into.
 * @return returns -1 if something went wrong else 0.
*/
static int write_data(char * buffer, FILE *out){
    while(fputs(buffer, out) == EOF){
        if(errno == EINTR)continue;
        return -1;
    }
    return 0;
}

/**
 * @brief Read data from children, performs calculations and writes result to stdout.
 * @param q fd from which to read Re from
 * @param t fd from which to read Ro from
 * @param n total expected output size.
 */
static void calculate_result(int q, int t, int n){
    char buffer[MAX_LINE_LENGTH];
    complex_t e, o;
    complex_t result[n];
    FILE * eIn = fdopen(q,"r");
    FILE * oIn = fdopen(t,"r");
    
    for(int i = 0; i < n/2;i++){
        if(read_data(buffer, eIn) == -1){
            fclose(eIn);
            fclose(oIn);
            error_exit("Input has to be even and contain only valid floating point numbers!");
        }

        string_to_imaginary(buffer, &e);
        
        if(read_data(buffer, oIn) == -1){
            fclose(oIn);
            fclose(eIn);
            error_exit("Input has to be even and contain only valid floating point numbers");
        };

        string_to_imaginary(buffer, &o);
        
        butterfly(&e,&o,i,n);
        result[i]=e;
        result[i+n/2]=o;
    }
    fclose(eIn);
    fclose(oIn);
    for(int i = 0; i < n;i++){
        snprintf(buffer, MAX_LINE_LENGTH, "%f %f*i\n",result[i].real,result[i].imaginary);
        write_data(buffer,stdout);
    }
}
/**
 * Main
*/
int main(int argc, char * argv[]){
    program = argv[0];

    int size;
    char bufferA[MAX_LINE_LENGTH];
    char bufferB[MAX_LINE_LENGTH];
    // Buffering two lines to check for termination condition
    if(read_data(bufferA, stdin) == -1){
        error_exit("Failed to read!");
    }
    if(read_data(bufferB, stdin) == -1){
        //We can't get interrupted here!
        write_data(bufferA,stdout);
        exit(EXIT_SUCCESS);
    }
    // Forking and saving info in info_t
    info_t eInfo, oInfo;
    spawn_child(&eInfo);
    spawn_child(&oInfo);
    FILE * fileE = fdopen(eInfo.write, "w");
    FILE * fileO = fdopen(oInfo.write, "w");

    // Writing two buffered inputs
    size = 2;
    if(write_data(bufferA, fileE) == -1){
        error_exit("Failed to write");
    }
    if(write_data(bufferB, fileO) == -1){
        error_exit("Failed to write");
    }
    // works
    while(read_data(bufferA,stdin) != -1){
        if(write_data(bufferA,fileE) == -1){
            error_exit("Failed to write");
        }
        if(read_data(bufferB, stdin)!=-1){
            if(write_data(bufferB,fileO) == -1){
                error_exit("Failed to write!");
            }
        } else {
            close(oInfo.read);
            close(eInfo.read);
            fclose(fileE);
            fclose(fileO);
            error_exit("Input has to be even!");
        }
        size+=2;
    }
    fclose(fileE);
    fclose(fileO);
    // Checking if child processes terminated correctly
    int status;
    waitpid(eInfo.pid,&status,0);
    if(WIFEXITED(status)){
        if(WEXITSTATUS(status) == EXIT_FAILURE){
            close(eInfo.read);
            close(oInfo.read);
            error_exit("Child Process failed!");
        }
    }
    waitpid(oInfo.pid,&status,0);
    if(WIFEXITED(status)){
        if(WEXITSTATUS(status) == EXIT_FAILURE){
            close(eInfo.read);
            close(oInfo.read);
            error_exit("Child Process failed!");
        }
    }
    
    calculate_result(eInfo.read,oInfo.read,size);
    exit(EXIT_SUCCESS);
}
