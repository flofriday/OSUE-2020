/**
 * @file sharedmem.h
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 31.10.2020
 *
 * @brief Provides functions to easily setup shared memory 
 * 
 * The sharedmem module. It conatains the shm struct and two functions to 
 * setup the memory sharing and to free those resources again.
 **/

#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <stdbool.h>

/**
 * @brief The size (in bytes) of data, NOT the size of the complete struct shm
 */
#define SHM_SIZE (2048)

/**
 * Structure of the shared memory
 * @brief All fields inside this struct are part of the shared memory.
 */
struct shm
{
	bool alive;
	size_t readpos;
	size_t writepos;
	char data[SHM_SIZE];
};

/**
 * Shared memory open function
 * @brief This function creates/connects to the shared memory, depending on the
 * role of the caller.
 * @details The value of shmfd might be modified, even tough the function
 * returned with NULL, in which case the file descriptor is invalid and should 
 * not be used. 
 * In general the filedescriptor should not be used to read or 
 * write data to the shared memory, but should only be saved unitl 
 * close_sharedmem gets called. 
 * The caller must not free() the struct returned by this function, if it is no 
 * longer needed, but must call close_sharedmem to free the resources.
 * At any point there should only be one shm struct per process
 * that got created with this function.
 * A server process must call this function first, before any client process 
 * calls it, because the server creates some resources to which the client just
 * connects.
 * @param shmfd A pointer to a filedescriptor. This function will overwrite 
 * the value with a filedescriptor to the shared memory.
 * @param role The role of the calling process. Allowed values are 's' for 
 * server and 'c' for client. There can only be one server, but many clients. 
 * @return Upon success this function will return a pointer to shm struct which 
 * might be shared with other processes. Otherwise NULL is returned.
 */
struct shm *open_sharedmem(int *shmfd, char role);

/**
 * Shared memory close function
 * @brief This function closes the connectio and maybe deletes the shared memory
 * depending on the role of the caller.
 * @details After this call the pointer shm will point to invalid memory, and 
 * reading from or writing to it is udefined behaivor.
 * @param shm A pointer to a sharedmem struct created with open_sharedmem
 * @param shmfd A file descriptor created by open_sharedmem
 * @param role The role of the calling process. Allowed values are 's' for 
 * server and 'c' for client. There can only be one server, but many clientsn. 
 * @return Upon success this function will return 0, otherwise -1.
 */
int close_sharedmem(struct shm *shm, int shmfd, char role);

#endif
