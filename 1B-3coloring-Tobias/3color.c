/**
 * @file   3color.c
 * @author Tobias de Vries (e01525369)
 * @date   20.11.2020
 *
 * @brief Implements the functions specified in 3color.h.
 **/

#include "3color.h"


/** For documentation see 3color.h - initializeSharedMemoryAsServer & initializeSharedMemoryAsClient
 *  The function alternates between the described functionalities depending on the value of asServer. */
static inline void initializeSharedMemory(int *shared_memory_fd_out, shared_memory_t **shared_memory_out, bool asServer)
{
    int openingOption = asServer ? OPEN_SHM_OPTION_SERVER : OPEN_SHM_OPTION_CLIENT;
    *shared_memory_fd_out = shm_open(SHARED_MEMORY_NAME, openingOption, OPEN_SHM_MODE);
    exitOnFailure(*shared_memory_fd_out, asServer ? OPENING_SHM_ERROR_SERVER : OPENING_SHM_ERROR_CLIENT);

    if (asServer)
        exitOnFailure(ftruncate(*shared_memory_fd_out, sizeof(shared_memory_t)), TRUNCATING_SHM_ERROR);

    *shared_memory_out = mmap(NULL, sizeof(shared_memory_t), SHM_MAP_PROT, SHM_MAP_FLAGS, *shared_memory_fd_out, MAPPING_SHM_OFFSET);
    if (*shared_memory_out == MAP_FAILED)
        printErrnoAndTerminate(MAPPING_SHM_ERROR);
}

/** For documentation see 3color.h - cleanupSharedMemoryAsServer & cleanupSharedMemoryAsClient
 *  The function alternates between the described functionalities depending on the value of asServer. */
static inline void cleanupSharedMemory(int shared_memory_fd, shared_memory_t *shared_memory, bool asServer)
{
    exitOnFailure(munmap(shared_memory, sizeof(*shared_memory)), UNMAPPING_SHM_ERROR);
    exitOnFailure(close(shared_memory_fd), CLOSING_SHM_ERROR);

    if (asServer)
        exitOnFailure(shm_unlink(SHARED_MEMORY_NAME), UNLINKING_SHM_ERROR);
}

/** For documentation see 3color.h - initializeSemaphoreAsServer & initializeSemaphoreAsClient
 *  The function alternates between the described functionalities depending on the value of asServer. */
static inline void initializeSemaphore(sem_t **semaphore_out, const char *semaphoreName, bool asServer, unsigned int initialValue)
{
    if (asServer)
        *semaphore_out = sem_open(semaphoreName, OPEN_SEM_OPTION_SERVER, OPEN_SEM_MODE, initialValue);
    else
        *semaphore_out = sem_open(semaphoreName, OPEN_SEM_OPTION_CLIENT);

    if (*semaphore_out == SEM_FAILED)
        printErrnoAndTerminate(OPENING_SEM_ERROR);
}

/** For documentation see 3color.h - cleanupSemaphoreAsServer & cleanupSemaphoreAsClient
 *  The function alternates between the described functionalities depending on the value of asServer. */
static inline void cleanupSemaphore(sem_t *semaphore, const char *semaphoreName, bool asServer)
{
    exitOnFailure(sem_close(semaphore), CLOSING_SEM_ERROR);

    if (asServer)
        exitOnFailure(sem_unlink(semaphoreName), UNLINKING_SEM_ERROR);
}


/** For documentation see 3color.h */
void initializeSharedMemoryAsServer(int *shared_memory_fd_out, shared_memory_t **shared_memory_out) {
    initializeSharedMemory(shared_memory_fd_out, shared_memory_out, true);
}

/** For documentation see 3color.h */
void initializeSharedMemoryAsClient(int *shared_memory_fd_out, shared_memory_t **shared_memory_out) {
    initializeSharedMemory(shared_memory_fd_out, shared_memory_out, false);
}

/** For documentation see 3color.h */
void cleanupSharedMemoryAsServer(int shared_memory_fd, shared_memory_t *shared_memory) {
    cleanupSharedMemory(shared_memory_fd, shared_memory, true);
}

/** For documentation see 3color.h */
void cleanupSharedMemoryAsClient(int shared_memory_fd, shared_memory_t *shared_memory) {
    cleanupSharedMemory(shared_memory_fd, shared_memory, false);
}

/** For documentation see 3color.h */
void initializeSemaphoreAsServer(sem_t **semaphore_out, const char *semaphoreName, unsigned int initialValue) {
    initializeSemaphore(semaphore_out, semaphoreName, true, initialValue);
}

/** For documentation see 3color.h */
void initializeSemaphoreAsClient(sem_t **semaphore_out, const char *semaphoreName) {
    initializeSemaphore(semaphore_out, semaphoreName, false, 0); // initialValue ignored for clients
}

/** For documentation see 3color.h */
void cleanupSemaphoreAsServer(sem_t *semaphore, const char *semaphoreName) {
    cleanupSemaphore(semaphore, semaphoreName, true);
}

/** For documentation see 3color.h */
void cleanupSemaphoreAsClient(sem_t *semaphore) {
    cleanupSemaphore(semaphore, NULL, false);
}
