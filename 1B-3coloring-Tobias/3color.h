/**
 * @file   3color.h
 * @author Tobias de Vries (e01525369)
 * @date   20.11.2020
 *
 * @brief Provides types and initialization methods shared by generator and supervisor as well as general Options.
 **/

#ifndef _3COLOR_H
#define _3COLOR_H

//region INCLUDES
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include "util.h"
//endregion

//region DEFINES

#define FOUND_0_EDGE_SOLUTION_MSG "... You know what? This graph seems to be 3-colorable!\n"

//region SHARED MEMORY OPTIONS
#define MAX_SOLUTION_SIZE        8  //for values too big for an int8 adjust type of solution_t.numberOfEdges
#define SOLUTION_BUFFER_SIZE     10 //for values too big for an int8 adjust type of shared_memory_t.currentWriteIndex
#define SOLUTION_EDGE_ARRAY_SIZE sizeof(((solution_t *)0)->edges)

#define SHARED_MEMORY_NAME     "/01525369_3colorBuffer"
#define OPEN_SHM_MODE          S_IRWXU                // NOLINT(hicpp-signed-bitwise)
#define SHM_MAP_PROT           PROT_READ | PROT_WRITE // NOLINT(hicpp-signed-bitwise)
#define SHM_MAP_FLAGS          MAP_SHARED
#define MAPPING_SHM_OFFSET     0
#define OPEN_SHM_OPTION_SERVER O_CREAT | O_RDWR       // NOLINT(hicpp-signed-bitwise)
#define OPEN_SHM_OPTION_CLIENT O_RDWR
//endregion

//region SEMAPHORE OPTIONS
#define FREE_SPACE_SEM_NAME "/01525369_freeSpaceSem"
#define FREE_SPACE_SEM_INIT MAX_SOLUTION_SIZE

#define USED_SPACE_SEM_NAME "/01525369_usedSpaceSem"
#define USED_SPACE_SEM_INIT 0

#define EXCL_WRITE_SEM_NAME "/01525369_mutuallyExclusiveWriteSem"
#define EXCL_WRITE_SEM_INIT 1

#define OPEN_SEM_MODE          S_IRWXU // NOLINT(hicpp-signed-bitwise)
#define OPEN_SEM_OPTION_CLIENT O_RDWR
#define OPEN_SEM_OPTION_SERVER O_CREAT | O_RDWR // NOLINT(hicpp-signed-bitwise)
//endregion

//region ERROR_MESSAGES
#define OPENING_SHM_ERROR_SERVER "Creating shared memory failed"
#define OPENING_SHM_ERROR_CLIENT "Opening shared memory failed. Ensure a supervisor is running"

#define TRUNCATING_SHM_ERROR "Initializing shared memory failed"
#define MAPPING_SHM_ERROR    "Mapping shared memory failed"

#define CLOSING_SHM_ERROR    "Closing shared memory file failed"
#define UNMAPPING_SHM_ERROR  "Unmapping shared memory failed"
#define UNLINKING_SHM_ERROR  "Unlinking shared memory failed"

#define OPENING_SEM_ERROR   "Opening semaphore failed"
#define CLOSING_SEM_ERROR   "Closing semaphore failed"
#define UNLINKING_SEM_ERROR "Unlinking semaphore failed"
//endregion
//endregion

//region TYPES
typedef struct {
    int8_t color;
    unsigned long nodeIndex;
} node_t;

typedef struct {
    unsigned long firstNodeIndex;
    unsigned long secondNodeIndex;
} edge_t;

typedef struct {
    edge_t edges[MAX_SOLUTION_SIZE];
    int8_t numberOfEdges; // type must be adjusted if MAX_SOLUTION_SIZE gets too big for an int8
} solution_t;

typedef struct {
    bool       shutdownRequested;
    int8_t     currentWriteIndex; // type must be adjusted if SOLUTION_BUFFER_SIZE gets too big for an int8
    solution_t solutions[SOLUTION_BUFFER_SIZE];
} shared_memory_t;
//endregion

//region DECLARATIONS
/**
 * @brief Creates and initializes a shared memory.
 * @details Calls the functions shm_open(3), ftruncate(2) and mmap(2) with the above specified options
 *          (see section SHARED MEMORY OPTIONS) to do so.
 *          Terminates the program with EXIT_FAILURE upon failure of any of those functions
 *          and prints the corresponding error.
 *
 * @param shared_memory_fd_out The pointer that should point to the created file descriptor.
 * @param shared_memory_out    The pointer that should point to a pointer pointing to the created shared memory.
 */
void initializeSharedMemoryAsServer(int *shared_memory_fd_out, shared_memory_t **shared_memory_out);

/**
 * @brief Opens a shared memory in a client.
 * @details Calls the functions shm_open(3) and mmap(2) with the above specified options
 *          (see section SHARED MEMORY OPTIONS) to do so.
 *          Terminates the program with EXIT_FAILURE upon failure of any of those functions
 *          and prints the corresponding error.
 *
 * @param shared_memory_fd_out The pointer that should point to the created file descriptor.
 * @param shared_memory_out    The pointer that should point to a pointer pointing to the shared memory.
 */
void initializeSharedMemoryAsClient(int *shared_memory_fd_out, shared_memory_t **shared_memory_out);

/**
 * @brief Closes and deletes a shared memory.
 * @details Calls the functions munmap(2), close(2) and shm_unlink(3) with the above specified options
 *          (see section SHARED MEMORY OPTIONS) to do so.
 *          Terminates the program with EXIT_FAILURE upon failure of any of those functions
 *          and prints the corresponding error.
 *
 * @param shared_memory_fd The file descriptor corresponding to the shared memory.
 * @param shared_memory    The pointer to the shared memory that should be released.
 */
void cleanupSharedMemoryAsServer(int shared_memory_fd, shared_memory_t *shared_memory);


/**
 * @brief Closes a shared memory created by a server.
 * @details Calls the functions munmap(2) and close(2) with the above specified options
 *          (see section SHARED MEMORY OPTIONS) to do so.
 *          Terminates the program with EXIT_FAILURE upon failure of any of those functions
 *          and prints the corresponding error.
 *
 * @param shared_memory_fd The file descriptor corresponding to the shared memory.
 * @param shared_memory    The pointer to the shared memory that should be closed.
 */
void cleanupSharedMemoryAsClient(int shared_memory_fd, shared_memory_t *shared_memory);

/**
 * @brief Creates and initializes a semaphore.
 * @details Calls the function sem_open(3) with the additional arguments mode and value to do so.
 *          Mode and oflag are set as specified in section SEMAPHORE OPTIONS above.
 *          Terminates the program with EXIT_FAILURE upon failure of sem_open and prints the corresponding error.
 *
 * @param semaphore_out The pointer that should point to a pointer pointing to the created semaphore.
 * @param semaphoreName The name of the semaphore. For details about its construction, see sem_overview(7).
 * @param initialValue  The value that the created semaphore should initially posses.
 */
void initializeSemaphoreAsServer(sem_t **semaphore_out, const char *semaphoreName, unsigned int initialValue);

/**
 * @brief Opens a semaphore created by a server.
 * @details Calls the function sem_open(3) with name and oflag arguments.
 *          oflag is set as specified in section SEMAPHORE OPTIONS above.
 *          Terminates the program with EXIT_FAILURE upon failure of sem_open and prints the corresponding error.
 *
 * @param semaphore_out The pointer that should point to a pointer pointing to the opened semaphore.
 * @param semaphoreName The name of the semaphore. For details about its construction, see sem_overview(7).
 */
void initializeSemaphoreAsClient(sem_t **semaphore_out, const char *semaphoreName);

/**
 * @brief Closes and deletes a semaphore.
 * @details Calls the functions sem_close(3) and sem_unlink(3) to do so.
 *          Terminates the program with EXIT_FAILURE upon failure of any of those functions
 *          and prints the corresponding error.
 *
 * @param semaphore     A pointer to the semaphore.
 * @param semaphoreName The name of the semaphore.
 */
void cleanupSemaphoreAsServer(sem_t *semaphore, const char *semaphoreName);


/**
 * @brief Closes a semaphore created by a server.
 * @details Calls the function sem_close(3) to do so.
 *          Terminates the program with EXIT_FAILURE upon failure of sem_close and prints the corresponding error.
 *
 * @param semaphore A pointer to the semaphore.
 */
void cleanupSemaphoreAsClient(sem_t *semaphore);
//endregion

#endif //_3COLOR_H
