/**
 * @file buffer.c
 * @author briemelchen
 * @date 03.11.2020
 * @brief Implementation of buffer.h
 * @details All operations as writing and reading to/from the buffer are handled by this module. All operations
 * are synchronized by semaphores which are also defined in the module. See more information @file buffer.h
 * 
 * 
 * last modified: 14.11.2020
 */
#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <errno.h>

#include <string.h>

#include <string.h>

#include <fcntl.h>

#include <sys/mman.h>

#include "buffer.h"

int supervisor_setup(void)
{
    // init semaphore which is initalized with buffer-length, because at start-up
    // there is as much space in the buffer to wrtie to so therefore the semaphore is
    // initalized with the buffer's length. O_CREAT in Combination with O_EXCL is used to get an error if the
    // semaphore already exists. mode 0600/S_IRUSR | S_IWUSR: Read and Write permissions for same user are in my opinion
    // sufficent.
    sem_free = sem_open(SEM_NAME_WR, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, BUFFER_LENGTH);
    if (sem_free == SEM_FAILED)
    {
        return -1;
    }

    // inits semaphore which is initalized with 0 which implies how many results are ready to get read by the supervisor.
    // 0 before a generator has written something to the buffer. Flags/Mode are same as before.
    sem_used = sem_open(SEM_NAME_RD, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    if (sem_used == SEM_FAILED)
    {
        return -1;
    }
    // inits semaphore which is inilaized with 1 to guarantee mutual exclusion when writing to the buffer.
    sem_w_block = sem_open(SEM_NAME_WR_BLOCK, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (sem_w_block == SEM_FAILED)
    {
        return -1;
    }

    // init the shared memory. Flags: O_RDWR: To allow read/write accesses. O_CREAT: if the shared mem does not exits, it gets created.
    // Mode 0600 as above: Read/Write permissions for the process-owning user
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1)
    {
        return -1;
    }

    // truncates the file (shared memory object) to the size of the buffer
    if (ftruncate(shmfd, sizeof(*buffer)) < 0)
    {
        return -1;
    }

    // maps the shared memory into the virtual address space: NULL as address so the kenerl decides which address is used
    // length is again the size of the buffer, PROT_READ | PROT_WRITE so writes and reads can be performed,
    // MAP_SHARED so updates of the memory is also updated by other processes which use the shared mem. NO offset is specified.
    buffer = mmap(NULL, sizeof(*buffer), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    if (buffer == MAP_FAILED)
    {
        return -1;
    }
    if (close(shmfd) != 0) // close file descriptor
    {
        return -1;
    }
    buffer->sig_state = 0; // state of buffer = 0 -> standard state: generators may write and supervisor may read.
    return shmfd;
}

int generator_setup(void)
{
    // opens the (already setuped) semaphores
    sem_used = sem_open(SEM_NAME_RD, 0);
    if (sem_used == SEM_FAILED)
    {
        return -1;
    }

    sem_free = sem_open(SEM_NAME_WR, 0);
    if (sem_free == SEM_FAILED)
    {
        return -1;
    }
    sem_w_block = sem_open(SEM_NAME_WR_BLOCK, 0);
    if (sem_w_block == SEM_FAILED)
    {
        return -1;
    }
    // opens the shared mem as specified in supervisor_setup
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1)
    {
        return -1;
    }

    // maps the buffer into the virtual address space
    buffer = mmap(NULL, sizeof(*buffer), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    // fprintf(stdout, "%ld", sizeof(*buffer));
    if (buffer == MAP_FAILED)
    {
        return -1;
    }

    if (close(shmfd) != 0) // close file descriptor
    {
        return -1;
    }

    return shmfd;
}

solution read_entry_from_buffer(void)
{
    solution solution = {.origin_edge_count = -1,
                         .removed_edges = -1};
    if (sem_wait(sem_used) == -1) // waits until there is an element to read in the buffer
    {
        return solution;
    }

    int r_pos = buffer->read_pos;
    //   printf("rpos %d\n", r_pos);
    solution = buffer->sol[r_pos];
    r_pos += 1;
    r_pos %= BUFFER_LENGTH;
    buffer->read_pos = r_pos;
    if (sem_post(sem_free))
    { // increments the free semaphore -> read entry can be overwritten
        solution.removed_edges = -1;
        return solution;
    }

    return solution;
}

int write_solution_buffer(solution solution)
{
    if (sem_wait(sem_free) == -1) // wait until there is space to write in the buffer
    {
        return -1;
    }

    if (sem_wait(sem_w_block) == -1) // garantees mutex for write operation
    {
        return -1;
    }
    if (get_state() == -1) // if state has changed meanwhile, the generator should not write anymore
    {
        if (sem_post(sem_w_block) == -1) // unblocks the mutex write semaphore
        {
            return -1;
        }
        return -2;
    }

    int w_pos = buffer->write_pos;
    buffer->sol[w_pos] = solution;
    w_pos += 1;
    w_pos %= BUFFER_LENGTH;
    buffer->write_pos = w_pos;
    if (sem_post(sem_used) == -1) // increment used semaphore
    {
        return -1;
    }
    if (sem_post(sem_w_block) == -1) // unblocks the mutex write semaphore
    {
        return -1;
    }
    return 0;
}

int get_state(void)
{
    int re_val = -1;
    re_val = buffer->sig_state;
    return re_val;
}

int set_state(int new_state)
{
    buffer->sig_state = new_state;
    //  printf("State setted to %d\n",new_state);
    return 0;
}
