/**
 * @file buffer.h
 * @author briemelchen
 * @date 03.11.2020
 * @brief Module which handles setup of the circular buffer as well as writing and reading from it.
 * @details All operations as writing and reading to/from the buffer are handled by this module. All operations
 * are synchronized by semaphores which are also defined in the module. Setup for the different programs (supervisor/generator)
 * are also provided in the module.
 * 
 * last modified: 14.11.2020
 */
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include "globals.h"

#ifndef circularbuffer_h
#define circularbuffer_h

sem_t *sem_free;       // semaphore which handles writing operations ("free space semaphore")
sem_t *sem_used;       // semaphore which handles reading operations ("used space semaphore")
sem_t *sem_w_block;    // semaphore which makes writing operations atomic so that only one process can write at the same time

circ_buffer *buffer; // buffer which holds the solutions and which is maped from the shared memory

/**
 * @brief setups the buffer and all semaphores for the supervisor.
 * @details setups the supervisor: opens all semaphores with according init-values (sem_free is initalized with
 * BUFFER_LENGTH (see globals.h), sem_used with 0 and sem_w_block with 1). Opens the shared memory and maps it into
 * the addressspace of the supervisor.
 * @return the file discriptor of the shared memory, in case of an error -1.
 */
int supervisor_setup(void);

/**
 * @brief setups the buffer and all semaphores for the generator-processes.
 * @details opens all semaphores (which have to be created by supervisor process in advance) and the shared memory.
 * Should only be called, if supervisor_setup() has been called before by a different process.
 * @return the file descriptor of the shared memory, in case of an error -1.
 */
int generator_setup(void);

/**
 * @brief reads an entry from the buffer.
 * @details reads an entry from the buffer. The function respectlively the semaphore  blocks as long as a generator
 * has written an entry to the buffer. If an entry has be written, the  semaphore unblocks and the method returns
 * an solution struct. After that, the semaphore which is used for tracking frees space is posted and the index of the writing position
 * is increased.
 * @return a solution struct, holding a valid 3-color-solution
 */
solution read_entry_from_buffer(void);

/**
 * @brief writes a solution to the buffer.
 * @details writes a valid solution of the 3-color-problem to the buffer. The semaphore  blocks as long as there
 * is free-space to write. After successfully writing to the buffer, the semaphore sem_used is posted for increasing the
 * used space of the buffer and the index of the writing position is increased.
 * @param solution  to be written
 * @return 0 on success, -1 on failure
 */
int write_solution_buffer(solution solution);

/**
 * @brief returns the actual state of the buffer.
 * @details the  semaphore blocks as long as get_state returns, so no writing process can fail due to
 * wrong state of the buffer.
 * @return State 0: Buffer is running, State -1: Supervisor indicated to stop the program, -2 on failure
 */
int get_state(void);

/**
 * @brief sets the state of the buffer, and therefore the state of the program.
 * @details sets the state of the buffer. Set_state is synchronized  via the write-semaphore  to avoid
 * blockings.
 * @param new_state which the buffer should be set to (-1 indicates generators to stop, 0 for "normal" program flow)
 * @return 0 on success, -1 on failure.
 */
int set_state(int new_state);

#endif