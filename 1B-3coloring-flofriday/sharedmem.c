/**
 * @file sharedmem.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 31.10.2020
 *
 * @brief Implementation of the sharedmem module.
 **/

#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "sharedmem.h"

/**
 * @brief The name for the shared memory
 */
#define SHM_NAME "/XXXXXXXX_osue_shm"

/**
 * @details uses the macros SHM_NAME and SHM_SIZE
 */
struct shm *open_sharedmem(int *shmfd, char role)
{
	assert(role == 'c' || role == 's');

	// Create the flags accourding to the role
	int mmap_flags = MAP_SHARED;
	int mmap_prot = PROT_READ | PROT_WRITE;
	int shm_flags = 0;

	if (role == 's')
	{
		shm_flags = O_RDWR | O_CREAT | O_EXCL;
	}
	else if (role == 'c')
	{
		shm_flags = O_RDWR;
	}

	// Create and/or open the shared memory object:
	*shmfd = shm_open(SHM_NAME, shm_flags, 0600);
	if (*shmfd == -1)
	{
		return NULL;
	}

	if (role == 's')
	{
		// set the size of the shared memory:
		if (ftruncate(*shmfd, sizeof(struct shm)) == -1)
		{
			close(*shmfd);
			return NULL;
		}
	}

	// Map shared memory object:
	struct shm *shm;

	shm = mmap(NULL, sizeof(struct shm), mmap_prot, mmap_flags, *shmfd, 0);
	if (shm == MAP_FAILED)
	{
		close(*shmfd);
		return NULL;
	}

	/* Init the fileds (only the server does this) */
	if (role == 's')
	{
		shm->alive = true;
		shm->readpos = 0;
		shm->writepos = 0;
		memset(shm->data, 0, SHM_SIZE);
	}

	return shm;
}

/**
 * @details uses the macro SHM_NAME
 */
int close_sharedmem(struct shm *shm, int shmfd, char role)
{
	assert(role == 'c' || role == 's');

	int ret_val = 0;

	// unmap shared memory:
	if (munmap(shm, sizeof(*shm)) == -1)
	{
		ret_val = -1;
	}

	// close the file descriptor
	if (close(shmfd) == -1)
	{
		ret_val = -1;
	}

	// remove shared memory object (only the server needs to do this)
	if (role == 's')
	{
		if (shm_unlink(SHM_NAME) == -1)
		{
			ret_val = -1;
		}
	}

	return ret_val;
}
