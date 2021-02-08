/**
 * @file circular_buffer.c
 * @author Jonny X (12345678) <e12345678@student.tuwien.ac.at>
 * @date 19.11.2020
 *
 * @brief The circular_buffer file acts like a Semaphore and Shared_memory library which helps the 
 * generator and supervisor files.
 * 
 **/

#include "circular_buffer.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#include <unistd.h>
#include <sys/types.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

//Shared memory constants
#define SHM_NAME "/12345678_shared_memory"

//Semaphore constants
#define SEM_FREE_NAME "/12345678_free"
#define SEM_USED_NAME "/12345678_used"
#define SEM_BLOCKED_NAME "/12345678_blocked"

sem_t *sem_free;
sem_t *sem_used;
sem_t *sem_blocked;

int shmfd;

/** open_shm function
 * @brief Opens the shared memory
 * 
 * @details Tries to open the shared memory. If one step fails it frees the allocated resources and exits with
 * an error codee.
 * 
 * @param is_server This bool specifies wheter the caller is a server or not. This decides if the shared memory should
 * be created (only done by server) or just opened (client).
 * 
 * @return Returns the shared memory struct if successful. Otherwise NULL is returned.
 */
static shared_memory *open_shm(bool is_server)
{
    if (is_server)
    {
        shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, 0600);
    }
    else
    {
        shmfd = shm_open(SHM_NAME, O_RDWR, 0600);
    }

    if (shmfd == -1)
    {
        return NULL;
    }

    // set the size of the shared memory:
    if (is_server)
    {
        if (ftruncate(shmfd, sizeof(shared_memory)) < 0)
        {
            close(shmfd);
            shm_unlink(SHM_NAME);
            return NULL;
        }
    }

    // map shared memory object:
    shared_memory *shm;
    shm = mmap(NULL, sizeof(*shm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm == MAP_FAILED)
    {
        close(shmfd);
        shm_unlink(SHM_NAME);
        return NULL;
    }

    return shm;
}

/** close_shm function
 * @brief Closes the shared memory
 * 
 * @details Tries to close the shared memory. If one step fails it still tries to complete the others and
 * then returns an exit code
 * 
 * @param is_server This bool specifies wheter the caller is a server or not. This decides if the shared memory should
 * be unlinked (only done by server) or not.
 * 
 * @return The function returns 0 if everything was successful. Otherwise -1 is returned.
 */
static int close_shm(shared_memory *shm, bool is_server)
{
    int exit_code = 0;

    if (munmap(shm, sizeof(*shm)) == -1)
    {
        exit_code = -1;
    }

    if (close(shmfd) == -1)
    {
        exit_code = -1;
    }

    if (is_server)
    {
        if (shm_unlink(SHM_NAME) == -1)
        {
            exit_code = -1;
        }
    }

    return exit_code;
}

/** open_semaphore function
 * @brief Opens all semaphores
 * 
 * @details Tries to open all semaphores. If an error occcures, it closes all already opened semaphores and the
 * exits the function with an error code.
 * 
 * @param is_server This bool specifies wheter the caller is a server or not. This decides if the semaphores should
 * be created or just opened.
 * 
 * @return The function returns 0 if everything was successful. Otherwise -1 is returned.
 */
static int open_semaphore(bool is_server)
{
    if (is_server)
    {
        sem_free = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, SHARED_MEMORY_MAX_DATA);
        if (sem_free == SEM_FAILED)
        {
            return -1;
        }

        sem_used = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0);
        if (sem_used == SEM_FAILED)
        {
            sem_close(sem_free);
            sem_unlink(SEM_FREE_NAME);
            return -1;
        }

        sem_blocked = sem_open(SEM_BLOCKED_NAME, O_CREAT | O_EXCL, 0600, 1);
        if (sem_blocked == SEM_FAILED)
        {
            sem_close(sem_free);
            sem_unlink(SEM_FREE_NAME);

            sem_close(sem_used);
            sem_unlink(SEM_USED_NAME);

            return -1;
        }
    }
    else
    {
        sem_free = sem_open(SEM_FREE_NAME, 0);
        if (sem_free == SEM_FAILED)
        {
            return -1;
        }

        sem_used = sem_open(SEM_USED_NAME, 0);
        if (sem_free == SEM_FAILED)
        {
            sem_close(sem_free);
            return -1;
        }

        sem_blocked = sem_open(SEM_BLOCKED_NAME, 0);
        if (sem_free == SEM_FAILED)
        {
            sem_close(sem_free);
            sem_close(sem_used);
            return -1;
        }
    }

    return 0;
}

/** close_semaphore function
 * @brief Closes all semaphores
 * 
 * @details Tries to close all semaphores. If an error occcures, it still tries to close the remaining 
 * semaphores.
 * 
 * @param is_server This bool specifies wheter the server functions should be called or not. This is 
 * import because ther are some actions only the Server should call (sem_unlink())
 * 
 * @return The function returns 0 if everything was successful. Otherwise -1 is returned.
 */
static int close_semaphore(bool is_server)
{
    int exit_code = 0;

    if (sem_close(sem_free) == -1)
    {
        exit_code = -1;
    }
    if (sem_close(sem_used) == -1)
    {
        exit_code = -1;
    }
    if (sem_close(sem_blocked) == -1)
    {
        exit_code = -1;
    }

    if (is_server)
    {
        if (sem_unlink(SEM_FREE_NAME) == -1)
        {
            exit_code = -1;
        }

        if (sem_unlink(SEM_USED_NAME) == -1)
        {
            exit_code = -1;
        }

        if (sem_unlink(SEM_BLOCKED_NAME) == -1)
        {
            exit_code = -1;
        }
    }

    return exit_code;
}




//public functions


/** open_circular_buffer function
 * @brief Helper functions which the supervisor and generators use to open the circular buffer.
 * 
 * @details The function opens the shared memory and then the semaphores. If something fails it frees the
 * resources and exits the function.
 * 
 * @param is_server This bool specifies wheter the server functions should be called or not. This is 
 * import because ther are some actions only the Server should call (e.g. ftruncate())
 * 
 * @return shared_memory The shared_memory struct is returned if everything was successful. Otherwise NULL is returned.
 */
shared_memory *open_circular_buffer(bool is_server)
{
    shared_memory *shm = open_shm(is_server);
    if (shm == NULL)
    {
        return NULL;
    }

    if (open_semaphore(is_server) == -1)
    {
        close_shm(shm, is_server);
        return NULL;
    }

    if (is_server)
    {
        shm->read_pos = 0;
        shm->write_pos = 0;
        shm->generators_should_quit = false;
    }

    return shm;
}


/** write_circular_buffer function
 * @brief Helper functions which the generators use to write to the circular buffer.
 * 
 * @details This functions uses sem_wait() and sem_post() to make sure that only one generator at a time writes 
 * to the circular buffer.
 * 
 * @param shm The shared memory object which open_circular_buffer returns.
 * 
 * @param data The solution from a generator which should be written to the circular buffer.
 * 
 * @return Returns 0 if everything was successful. Otherwise -1 is returned.
 */
int write_circular_buffer(shared_memory *shm, const char *data)
{
    //strings are null terminated
    if (sem_wait(sem_blocked) == -1)
    {
        return -1;
    }

    for (int i = 0; shm->generators_should_quit == false; ++i)
    {
        if (sem_wait(sem_free) == -1)
        {
            sem_post(sem_blocked);
            return -1;
        }
        //printf("WRITING:%c\n", data[i]);
        char symbol = data[i];
        shm->buffer[shm->write_pos] = symbol;
        shm->write_pos += 1;
        shm->write_pos %= SHARED_MEMORY_MAX_DATA;
        sem_post(sem_used);

        if (data[i] == 0)
        {
            break;
        }
    }
    sem_post(sem_blocked);

    if (shm->generators_should_quit)
    {
        return -1;
    }

    return 0;
}

/** read_circular_buffer function
 * @brief Helper functions which the supervisor uses to read the circular buffer.
 * 
 * @details The function call uses the semaphores and shared memory to read the solution from
 * the circular buffer. It allocates more memory if necessary. 
 * 
 * @param shm The shared_memory struct which open_circular_buffer returned.
 * 
 * @return Returns the read solution from the circular buffer
 */
char *read_circular_buffer(shared_memory *shm)
{
    int cap = 50;
    char *data = malloc(sizeof(char) * cap);
    for (int i = 0;; ++i)
    {

        if (sem_wait(sem_used) == -1)
        {
            free(data);
            return NULL;
        }

        if (i == cap)
        {
            cap = cap * 2;
            data = realloc(data, cap);
        }

        data[i] = shm->buffer[shm->read_pos];
        shm->read_pos += 1;
        shm->read_pos %= SHARED_MEMORY_MAX_DATA;
        sem_post(sem_free);

        //exit if end of string
        if (data[i] == 0)
        {
            break;
        }
    }

    return data;
}


/** close_circular_buffer function
 * @brief Helper functions which the supervisor and generators use to close the circular buffer.
 * 
 * @details The function closes the shared memory and then the semaphores. 
 * If something fails it still tires to free the other resources and then exist with an exit code.
 * 
 * @param shm The shared_memory struct which open_circular_buffer returned.
 * @param is_server This bool specifies wheter the server functions should be called or not. This is 
 * import because ther are some actions only the Server should call (e.g. sem_unlink())
 * 
 * @return The function returns 0 if everything was successful. Otherwise -1 is returned.
 */
int close_circular_buffer(shared_memory *shm, bool is_server)
{
    int exit_code = 0;

    sem_post(sem_free);
    shm->generators_should_quit = true;

    if (close_shm(shm, is_server) == -1)
    {
        exit_code = -1;
    }
    if (close_semaphore(is_server) == -1)
    {
        exit_code = -1;
    }

    return exit_code;
}
