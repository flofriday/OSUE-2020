/**
 * @file circbuf.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 31.10.2020
 *
 * @brief Provides functions to create, delete, read from and write to a shared 
 * circular buffer
 * 
 * The circbuf module. It contains easy to use functions for many clients to 
 * communicate with one server. While the server can only read the data, the 
 * clients can only write data.
 **/

#ifndef CIRCBUF_H
#define CIRCBUF_H

#include <semaphore.h>
#include "sharedmem.h"

/**
 * Structure of the circular buffer
 * @brief Internal structure of the circular buffer
 * @details Only the shm struct inside the circbuf struct is shared with other 
 * processes
 */
struct circbuf
{
	struct shm *shm;
	int shmfd;
	sem_t *s_free;
	sem_t *s_used;
	sem_t *s_write;
};

/**
 * Open the circular buffer.
 * @brief This function creates a shared circular buffer. Depending on the role
 * it might just connect to an already existing shared memory or create the 
 * resources.
 * @details This function must be called by a server process, before it is 
 * called by any client process, as the server has to create the resources, 
 * while the clients just connect to them.
 * At any point there should only be one circbuf struct per process that got 
 * created with this function.
 * The caller must not free() the returned circbuf struct, but must instead 
 * call close_circbuf to free the resources.
 * The caller should not directly use this struct to read or write data from or 
 * to it but should use write_circbuf and read_circbuf to avoid race-conditions 
 * and memory corruption.
 * @param role The role of the calling process. Allowed values are 's' for 
 * server and 'c' for client. There can only be one server, but many clients. 
 * @return Upon success a pointer to a shared circular buffer is returned.
 * Otherwise NULL.
 */
struct circbuf *open_circbuf(char role);

/**
 * Close the circular buffer.
 * @brief This function closes the connection to the circular buffer. Depending 
 * on the role it might also delete the resources or just disconnect from them.
 * @details After this call the circbuf pointer is invalid and should no longer 
 * be used. Still reading/writing to/from it is undefined behaivor and will
 * likly result in a segmentation fault or memory corruption.
 * @param circbuf A pointer to a circbuf struct created with open_circbuf.
 * @param role The role of the calling process. Allowed values are 's' for 
 * server and 'c' for client. There can only be one server, but many clients. 
 * @return 0 if all closing operation are successfull, otherwise -1.
 */
int close_circbuf(struct circbuf *circbuf, char role);

/**
 * Write a string to the circular buffer
 * @brief This function writes a null-terminated string to the buffer.
 * @details This function blocks till the whole string is written to the buffer.
 * In other words, it is very slow and should be used as little as possible.
 * This function might be called by many client processes.
 * @param circbuf A pointer to a circbuf struct created with open_circbuf.
 * @param content The string to be written to the buffer.
 */
void write_circbuf(struct circbuf *circbuf, char *content);

/**
 * Read a string from the circular buffer
 * @brief This function reads a null-terminated string from the circular buffer 
 * and returns it.
 * @details This function blocks till a whole string was read from the buffer.
 * The returned string must be freed by the caller.
 * This function should only be called by a single server process.
 * @param circbuf A pointer to a circbuf struct created with open_circbuf.
 * @return Upon success return a pointer to string with the read message. 
 * Otherwise NULL.
 */
char *read_circbuf(struct circbuf *circbuf);

#endif
