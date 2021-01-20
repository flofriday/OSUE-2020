/**
 * @file forkFFT.h
 * @author <xxxxxx@tuwien.ac.at>
 * @date 17.12.2020
 * 
 * @brief Header file containing structs used in forkFFT.c
 */
#include <stdlib.h> 
#define MAX_LINE_LENGTH (128)
#define PI (3.141592654)

/**
 * @brief stores two floats compromising the real and the imaginary part of a complex number.
 * @param real real part of complex number
 * @param imaginary imaginary part of complex number.
*/
typedef struct ComplexNumber {
    float real;
    float imaginary;
} complex_t;

/**
 * @brief stores info about child after fork
 * @param read read is the fd from which we can read from child stdout.
 * @param write is the fd to which we can write to the childs stdin.
 * @param pid is the pid of the child process.
*/
typedef struct Info {
    int read;
    int write;
    pid_t pid;
} info_t;
