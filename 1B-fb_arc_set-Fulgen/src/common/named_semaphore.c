/**
 * @file named_semaphore.c
 * @author George Tokmaji <e11908523@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief Named semaphore helper.
 *
 * This module deals with named semaphores.
 **/


#include "named_semaphore.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief The named_semaphore opaque struct.
 */
struct named_semaphore
{
	char *name; /**< The semaphore name **/
	sem_t *named_semaphore; /**< The semaphore **/
	bool created; /**< Whether the semaphore has been created or just opened **/
};

struct named_semaphore *named_semaphore_create(const char *name, int value)
{
	struct named_semaphore *semaphore;
	if ((semaphore = malloc(sizeof(struct named_semaphore))) == NULL)
	{
		goto malloc_error;
	}

	if ((semaphore->named_semaphore = sem_open(name, O_RDWR, S_IRUSR | S_IWUSR, value)) == NULL)
	{
		/* No semaphore found? Create it - separate step to unlink it later */
		if (errno != ENOENT || (semaphore->named_semaphore = sem_open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, value)) == NULL)
		{
			goto sem_open_error;
		}
		else
		{
			semaphore->created = true;
		}
	}
	else
	{
		semaphore->created = false;
	}

	semaphore->name = strdup(name);

	return semaphore;

sem_open_error:
	free(semaphore);
malloc_error:
	return NULL;
}

void named_semaphore_destroy(struct named_semaphore *semaphore)
{
	sem_close(semaphore->named_semaphore);
	if (semaphore->created)
	{
		sem_unlink(semaphore->name);
	}

	free(semaphore->name);
	free(semaphore);
}

int named_semaphore_wait(struct named_semaphore *semaphore)
{
	return sem_wait(semaphore->named_semaphore);
}

int named_semaphore_post(struct named_semaphore *semaphore)
{
	return sem_post(semaphore->named_semaphore);
}
