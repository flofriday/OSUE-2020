/**
 * @file circular_buffer.h
 * @author Jonny X (12345678) <e12345678@student.tuwien.ac.at>
 * @date 21.11.2020
 *
 * @brief The circular_buffer file acts like a Semaphore and Shared_memory library which helps the 
 * generator and supervisor files.
 * 
 **/

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>

#define SHARED_MEMORY_MAX_DATA (1024)

//solutions bigger than 8 edges are generally not accepted
#define MAX_SOLUTION_EDGE_COUNT 8




/** Shared memory struct
 * @brief The struct for the shared memory. Which is used for the circular buffer
 * 
 * @details Consists of
 * buffer - which holds the edge data.
 * read_pos - the current read position.
 * write_pos - the current write position.
 * generators-Should_quit - bool which signals if the supervisor 
 * finished and therfore all generators should quit
 * 
 */
typedef struct shared_memory
{
    char buffer[SHARED_MEMORY_MAX_DATA];
    int read_pos;
    int write_pos;
    bool generators_should_quit;
} shared_memory;




/** open_circular_buffer function
 * @brief Helper functions which the supervisor and generators use to open the circular buffer.
 * 
 * @details The function call multiple internal functions to open the semaphores and shared memory 
 * accordingly.
 * 
 * @param is_server This bool specifies wheter the server functions should be called or not. This is 
 * import because ther are some actions only the Server should call (e.g. ftruncate())
 * 
 * @return shared_memory The shared_memory struct is returned if everything was successful. Otherwise NULL is returned.
 */
shared_memory *open_circular_buffer(bool is_server);




/** write_circular_buffer function
 * @brief Helper functions which the generators use to write to the circular buffer.
 * 
 * @details The function call uses the semaphores and shared memory to write to the circular buffer.
 * 
 * @param shm The shared memory object which open_circular_buffer returns.
 * 
 * @param data The solution from a generator which should be written to the circular buffer.
 * 
 * @return Returns 0 if everything was successful. Otherwise -1 is returned.
 */
int write_circular_buffer(shared_memory *shm, const char *data);



/** read_circular_buffer function
 * @brief Helper functions which the supervisor uses to read the circular buffer.
 * 
 * @details The function call uses the semaphores and shared memory to read the solution from
 * the circular buffer.
 * 
 * @param shm The shared_memory struct which open_circular_buffer returned.
 * 
 * @return Returns the read solution from the circular buffer
 */
char *read_circular_buffer(shared_memory *shm);




/** close_circular_buffer function
 * @brief Helper functions which the supervisor and generators use to close the circular buffer.
 * 
 * @details The function call multiple internal functions to close the semaphores and shared memory 
 * accordingly.
 * 
 * @param shm The shared_memory struct which open_circular_buffer returned.
 * @param is_server This bool specifies wheter the server functions should be called or not. This is 
 * import because ther are some actions only the Server should call (e.g. sem_unlink())
 * 
 * @return The function returns 0 if everything was successful. Otherwise -1 is returned.
 */
int close_circular_buffer(shared_memory *shm, bool is_server);

#endif