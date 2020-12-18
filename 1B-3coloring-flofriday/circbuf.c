/**
 * @file generator.c
 * @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
 * @date 31.10.202
 *
 * @brief Generator program module.
 * 
 * Generates 3-coloring solutions for a specified graph and submits them to a 
 * supervisor process.
 **/

#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sharedmem.h"
#include "circbuf.h"

/**
 * @brief Names of the semaphores
 */
#define SEM_FREE "/XXXXXXXX_osue_sem_free"
#define SEM_USED "/XXXXXXXX_osue_sem_used"
#define SEM_WRITE "/XXXXXXXX_osue_sem_write"

/**
 * @details Uses the macros SEM_FREE, SEM_USED, SEM_WRITE (from circbuf.c) and
 * SHM_SIZE (sharedmem.h).
 */
struct circbuf *open_circbuf(char role)
{
	assert(role == 'c' || role == 's');

	// Allocate memory for the circbuf struct
	struct circbuf *circbuf = malloc(sizeof(struct circbuf));
	if (circbuf == NULL)
	{
		return NULL;
	}

	// Initialize the circbuf fields
	circbuf->s_free = NULL;
	circbuf->s_used = NULL;
	circbuf->s_write = NULL;

	// Open the shared memory
	circbuf->shm = open_sharedmem(&circbuf->shmfd, role);
	if (circbuf->shm == NULL)
	{
		free(circbuf);
		return NULL;
	}

	// Open the semaphores
	// If however opening one semaphore fails we need to close all resources
	if (role == 's')
	{
		// (The server can create them if they do not exist yet, but will fail
		// if they already exist.)
		circbuf->s_free = sem_open(SEM_FREE, O_CREAT | O_EXCL, 0600, SHM_SIZE);
		if (circbuf->s_free == SEM_FAILED)
		{
			close_sharedmem(circbuf->shm, circbuf->shmfd, role);
			free(circbuf);
			return NULL;
		}
		circbuf->s_used = sem_open(SEM_USED, O_CREAT | O_EXCL, 0600, 0);
		if (circbuf->s_used == SEM_FAILED)
		{
			sem_close(circbuf->s_free);
			sem_unlink(SEM_FREE);
			close_sharedmem(circbuf->shm, circbuf->shmfd, role);
			free(circbuf);
			return NULL;
		}
		circbuf->s_write = sem_open(SEM_WRITE, O_CREAT | O_EXCL, 0600, 1);
		if (circbuf->s_write == SEM_FAILED)
		{
			sem_close(circbuf->s_free);
			sem_unlink(SEM_FREE);
			sem_close(circbuf->s_used);
			sem_unlink(SEM_USED);
			close_sharedmem(circbuf->shm, circbuf->shmfd, role);
			free(circbuf);
			return NULL;
		}
	}
	else
	{
		circbuf->s_free = sem_open(SEM_FREE, 0);
		if (circbuf->s_free == SEM_FAILED)
		{
			close_sharedmem(circbuf->shm, circbuf->shmfd, role);
			free(circbuf);
			return NULL;
		}
		circbuf->s_used = sem_open(SEM_USED, 0);
		if (circbuf->s_used == SEM_FAILED)
		{
			sem_close(circbuf->s_free);
			close_sharedmem(circbuf->shm, circbuf->shmfd, role);
			free(circbuf);
			return NULL;
		}
		circbuf->s_write = sem_open(SEM_WRITE, 0);
		if (circbuf->s_write == SEM_FAILED)
		{
			sem_close(circbuf->s_free);
			sem_close(circbuf->s_used);
			sem_unlink(SEM_USED);
			close_sharedmem(circbuf->shm, circbuf->shmfd, role);
			free(circbuf);
			return NULL;
		}
	}

	return circbuf;
}

/**
 * @details Uses the macros SEM_FREE, SEM_USED, SEM_WRITE (from circbuf.c) and
 * SHM_SIZE (sharedmem.h).
 */
int close_circbuf(struct circbuf *circbuf, char role)
{
	assert(role == 'c' || role == 's');

	// Increase the free semaphore to avoid a generator freezing in a deadlock.
	// Also communicate with the clients that the server is no longer alive.
	if (role == 's')
	{
		sem_post(circbuf->s_free);
		circbuf->shm->alive = false;
	}

	// Init the return value of this function.
	int ret_val = 0;

	// Close the shared memory
	if (close_sharedmem(circbuf->shm, circbuf->shmfd, role) < 0)
	{
		ret_val = -1;
	}

	// Close the semaphores
	if (sem_close(circbuf->s_free) == -1)
	{
		ret_val = -1;
	}
	if (sem_close(circbuf->s_used) == -1)
	{
		ret_val = -1;
	}
	if (sem_close(circbuf->s_write) == -1)
	{
		ret_val = -1;
	}

	// The server also unlinks the semaphores
	if (role == 's')
	{
		if (sem_unlink(SEM_FREE) == -1)
			ret_val = -1;
		if (sem_unlink(SEM_USED) == -1)
			ret_val = -1;
		if (sem_unlink(SEM_WRITE) == -1)
			ret_val = -1;
	}

	return ret_val;
}

/**
 * @details Uses the macro SHM_SIZE (sharedmem.h).
 */
void write_circbuf(struct circbuf *circbuf, char *content)
{
	// Lock the write mutex to ensure only one generator is writing at a time
	if (sem_wait(circbuf->s_write) == -1)
	{
		return;
	}

	// Write all characters to the buffer
	for (size_t i = 0; circbuf->shm->alive; i++)
	{
		// Wait for free space in the circular buffer
		if (sem_wait(circbuf->s_free) == -1)
		{
			sem_post(circbuf->s_write);
			return;
		};

		// Write the current char and increase the write position and increase
		// used space indicator
		circbuf->shm->data[circbuf->shm->writepos] = content[i];
		sem_post(circbuf->s_used);
		circbuf->shm->writepos += 1;
		circbuf->shm->writepos %= SHM_SIZE;

		// Exit the loop if we wrote the Zero-terminator of the string
		if (content[i] == 0)
		{
			break;
		}
	}

	// Unlock the write mutex
	sem_post(circbuf->s_write);
	return;
}

/**
 * @details Uses the macro SHM_SIZE (sharedmem.h).
 */
char *read_circbuf(struct circbuf *circbuf)
{
	// Allocate memory for the string we need to read
	size_t len = 0;
	size_t cap = 32;
	char *s = malloc(sizeof(char) * cap);
	if (s == NULL)
	{
		return NULL;
	}

	// Read until the null-terminated string ends
	while (true)
	{
		// Wait till there is some content to read
		if (sem_wait(circbuf->s_used) == -1)
		{
			free(s);
			return NULL;
		}

		// Copy the current character and increase the read position and the
		// free semaphore
		s[len] = circbuf->shm->data[circbuf->shm->readpos];
		sem_post(circbuf->s_free);
		len += 1;
		circbuf->shm->readpos += 1;
		circbuf->shm->readpos %= SHM_SIZE;

		// Double the capacity of the string if we are running out of space
		if (len == cap)
		{
			cap *= 2;
			s = realloc(s, sizeof(char) * cap);
		}

		// Exit the loop if a zero-terminator was read
		if (s[len - 1] == 0)
		{
			break;
		}
	}

	return s;
}
