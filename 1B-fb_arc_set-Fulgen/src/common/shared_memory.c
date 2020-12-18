#include "named_semaphore.h"
#include "shared_memory.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define SEM_FREE 0
#define SEM_USED 1
#define SEM_WRITE 2

static const char *const semaphore_names[] = {"free", "used", "write"}; /**< Semaphore names **/
#define MAX_SEMAPHORE_NAME_SIZE 6 /**< Maximum semaphore name size, hardcoded for convenience. **/

/**
 * @brief The shared_memory opaque struct.
 */
struct shared_memory
{
	char *name; /**< The shared memory file name **/
	int fd; /**< The shared memory file descriptor **/
	bool created; /**< Whether the shared memory file has been created or opened **/
	void *memory; /**< A pointer to the memory mapped by mmap **/
	size_t memory_map_size; /**< The memory map size **/
	volatile bool *quit; /**< Whether the application should quit **/

	bool is_supervisor; /**< Whether the application is a supervisor **/

	size_t read_pos; /**< The read position. Unsynchronized as there can only be one supervisor **/
	uint32_t *write_pos; /**< The write position. Fixed size as it's part of the shared memory **/
	char *buffer; /**< The circular buffer in the memory map **/
	size_t capacity; /**< How many feedback arc sets can be stored **/
	struct named_semaphore *semaphores[3]; /**< The named semaphores **/
};

/**
 * @brief Initializes the semaphore pointer with a valid pointer to a named semaphore
 * @param semaphore A pointer to a named semaphore pointer
 * @param index An index of both the semaphore name and the semaphore (see semaphore_names)
 * @param name A semaphore name buffer used to write the full name to. It should already contain a leading slash and a prefix.
 * @param offset The offset in the name buffer which the semaphore name should be inserted at
 * @param value A value to initialize the named semaphore with
 * @return 0 on success. semaphore points to a valid named_semaphore pointer.
 * @return -1 on failure. semaphore points to a NULL named_semaphore pointer. Check errno for details.
 */
static int initialize_semaphore(struct named_semaphore **semaphore, size_t index, char *name, size_t offset, int value)
{
	strcpy(name + offset + 1, semaphore_names[index]); // + 1 to account for the leading slash

	if ((semaphore[index] = named_semaphore_create(name, value)) == NULL)
	{
		return -1;
	}

	return 0;
}

struct shared_memory *shared_memory_create(const char *const prefix, const char *const name, bool is_supervisor, size_t feedback_arc_capacity)
{
	char *shared_memory_name;
	const size_t prefix_length = strlen(prefix);
	const size_t name_length = strlen(name);
	int fd;

	bool created = false;
	const size_t buffer_size = sizeof(bool) + sizeof(uint32_t) + feedback_arc_capacity * sizeof(struct feedback_arc_set);

	struct shared_memory *shared_memory;
	void *memory_map;
	char *semaphore_name;

	/* | leading slash | prefix | name | NUL */
	if ((shared_memory_name = malloc((1 + prefix_length + name_length + 1) * sizeof(char))) == NULL)
	{
		goto shared_memory_name_malloc_error;
	}

	*shared_memory_name = '/';
	strcpy(shared_memory_name + 1, prefix);
	strcpy(shared_memory_name + prefix_length + 1, name);

	if ((fd = shm_open(shared_memory_name, O_RDWR, S_IRUSR | S_IWUSR)) == -1)
	{
		if (errno != ENOENT || !is_supervisor || (fd = shm_open(shared_memory_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1)
		{
			goto shm_open_error;
		}
		else
		{
			created = true;
		}
	}

	if (ftruncate(fd, buffer_size) == -1)
	{
		// error in errno
		goto ftruncate_error;
	}

	if ((shared_memory = malloc(sizeof(struct shared_memory))) == NULL)
	{
		goto shared_memory_malloc_error;
	}

	if ((memory_map = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == (void *) -1)
	{
		goto mmap_error;
	}

	if ((semaphore_name = malloc(prefix_length + MAX_SEMAPHORE_NAME_SIZE + 1)) == NULL)
	{
		goto semaphore_name_malloc_error;
	}

	*semaphore_name = '/';

	strncpy(semaphore_name + 1, prefix, prefix_length); // no +1 as the NUL character shouldn't be copied

	if (initialize_semaphore(shared_memory->semaphores, SEM_FREE, semaphore_name, prefix_length, feedback_arc_capacity) == -1)
	{
		goto sem_free_error;
	}

	if (initialize_semaphore(shared_memory->semaphores, SEM_USED, semaphore_name, prefix_length, 0) == -1)
	{
		goto sem_used_error;
	}

	if (initialize_semaphore(shared_memory->semaphores, SEM_WRITE, semaphore_name, prefix_length, 1) == -1)
	{
		goto sem_write_error;
	}

	free(semaphore_name);

	shared_memory->name = shared_memory_name;
	shared_memory->fd = fd;
	shared_memory->created = created;
	shared_memory->memory = memory_map;
	shared_memory->memory_map_size = buffer_size;
	shared_memory->quit = memory_map;
	shared_memory->is_supervisor = is_supervisor;
	shared_memory->read_pos = 0;
	shared_memory->write_pos = (uint32_t *) ((char *) memory_map + sizeof(bool));
	*shared_memory->write_pos = 0;
	shared_memory->buffer = (char *) shared_memory->write_pos + sizeof(uint32_t);
	shared_memory->capacity = feedback_arc_capacity;

	if (*shared_memory->quit != true && *shared_memory->quit != false)
	{
		// garbage set as quit; set it to false
		*shared_memory->quit = false;
	}

	return shared_memory;

sem_write_error:
	named_semaphore_destroy(shared_memory->semaphores[SEM_USED]);
sem_used_error:
	named_semaphore_destroy(shared_memory->semaphores[SEM_FREE]);
sem_free_error:
	free(semaphore_name);
semaphore_name_malloc_error:
	munmap(memory_map, buffer_size);
mmap_error:
	free(shared_memory);
ftruncate_error:
shared_memory_malloc_error:
	close(fd);
	if (created)
	{
		shm_unlink(shared_memory_name);
	}
shm_open_error:
	free(shared_memory_name);
shared_memory_name_malloc_error:
	return NULL;
}

void shared_memory_destroy(struct shared_memory *memory)
{
	named_semaphore_destroy(memory->semaphores[SEM_WRITE]);
	named_semaphore_destroy(memory->semaphores[SEM_USED]);
	named_semaphore_destroy(memory->semaphores[SEM_FREE]);
	munmap(memory->memory, memory->memory_map_size);
	close(memory->fd);

	if (memory->created)
	{
		shm_unlink(memory->name);
	}

	free(memory->name);
	free(memory);
}

int shared_memory_request_quit(struct shared_memory *memory)
{
	if (!memory->is_supervisor)
	{
		errno = EPERM;
		return -1;
	}

	*memory->quit = true;
	return 0;
}

bool shared_memory_quit_requested(struct shared_memory *const memory)
{
	return *memory->quit;
}

static int wait_for_semaphore(struct named_semaphore *const semaphore, volatile bool *quit)
{
	for (int ret = 0; (ret = named_semaphore_wait(semaphore)); )
	{
		if (ret == -1)
		{
			if (errno == EINTR && *quit != 1)
			{
				continue;
			}

			return ret;
		}
	}

	return 0;
}

int shared_memory_write_feedback_arc_set(struct shared_memory *memory, struct feedback_arc_set *const arc_set)
{
	if (memory->is_supervisor)
	{
		errno = EPERM;
		return -1;
	}

	if (wait_for_semaphore(memory->semaphores[SEM_WRITE], memory->quit) == -1)
	{
		return -1;
	}

	if (wait_for_semaphore(memory->semaphores[SEM_FREE], memory->quit) == -1)
	{
		return -1;
	}

	memcpy(memory->buffer + *memory->write_pos * sizeof(struct feedback_arc_set), arc_set, sizeof(struct feedback_arc_set));
	*memory->write_pos = (*memory->write_pos + 1) % memory->capacity;

	return named_semaphore_post(memory->semaphores[SEM_USED]) == 0 && named_semaphore_post(memory->semaphores[SEM_WRITE]) == 0 ? 0 : -1;
}

int shared_memory_read_feedback_arc_set(struct shared_memory *memory, struct feedback_arc_set *arc_set)
{
	if (!memory->is_supervisor)
	{
		errno = EPERM;
		return -1;
	}

	if (wait_for_semaphore(memory->semaphores[SEM_USED], memory->quit) == -1)
	{
		return -1;
	}

	memcpy(arc_set, memory->buffer + memory->read_pos * sizeof(struct feedback_arc_set), sizeof(struct feedback_arc_set));
	memory->read_pos = (memory->read_pos + 1) % memory->capacity;

	return named_semaphore_post(memory->semaphores[SEM_FREE]);
}
