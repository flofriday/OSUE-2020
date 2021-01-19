#include "fb_arc_set.h"

static void track_solutions(void);
static edge_container read_buffer(void);
static void wait_read(void);
static void signal_read(void);
static void initialize(void);
static void shutdown(void);
static void handle_signal(int signal);
static void ERROR_EXIT(char* message, char* error_details);
static void ERROR_MSG(char* message, char* error_details);
static void USAGE(void);
static const char* PROGRAM_NAME;

/**
 * @file supervisor.c
 * @author Michael Huber 11712763
 * @date 12.11.2020
 * @brief OSUE Exercise 1B fb_arc_set
 * @details The supervisor sets up the shared memory and 
 * the semaphores and initializes the circular buffer 
 * required for the communication with the generators. 
 * It then waits for the generators to write solutions 
 * to the circular buffer.
 */


/** Shared memory circular buffer file descriptor */
static int shm_fd = -1;

/** Circular buffer for writing and reading solutions */
static circular_buffer *buf = NULL;

/** Semaphore representing space used in the shared buffer, used for signaling 
 * the supervisor that it can read data. */
static sem_t *sem_used = NULL;
/** Semaphore representing free space in the shared buffer, used for signaling
 * the generator that it can write data. */
static sem_t *sem_free = NULL;
/** Semaphore used by generators to ensure mutual exclusion while writing.  */
static sem_t *sem_mutex = NULL;

/**
 * @brief Sets up shared memory, keeps track of best solutions.
 * @details Calls initialize, track_solutions.
 * @see initialize, track_solutions
 */
int main(int argc, char const *argv[]) {
    PROGRAM_NAME = argv[0];

    if (argc != 1){
        USAGE();
    }

    initialize();

    track_solutions();

    exit(EXIT_SUCCESS);
}

/**
 * @brief Keeps track of the best solution produced by the generators.
 * @details Keeps track of the best solution and prints it every time it receives 
 * a better one. Calls read_buffer; uses global variables buf.
 * @see read_buffer
 */
static void track_solutions() {
    edge_container solution = { .counter = SIZE_MAX };
    while(buf -> terminate == 0) {
        edge_container candidate = read_buffer();
        if (candidate.counter == 0) {
            printf("The graph is acyclic!\n");
            buf -> terminate = 1;
        } else if (candidate.counter < solution.counter) {
            solution = candidate;
            printf("Solution with %zu edges:", solution.counter);
            for (size_t i = 0; i < solution.counter; i++)
            {
                printf(" %ld-%ld", solution.container[i].u, solution.container[i].v);
            }
            printf("\n");
        }
    }
}

/**
 * @brief Reads new solution from the circular buffer and returns it. Blocks
 * until there is new data to read.
 * @details Calls wait_read, signal_read; uses global variables buf.
 * @see wait_read, signal_read
 * @return edge_container containing a solution candidate
 */
static edge_container read_buffer() {
    wait_read();
    edge_container candidate = buf->data[buf->read_pos];
    buf->read_pos = (buf->read_pos + 1) % BUF_SIZE;
    signal_read();
    return candidate;
}

/**
 * @brief Blocks until there is new data in the buffer to read.
 * @details Uses global variable sem_used.
 */
static void wait_read() {
    if(sem_wait(sem_used) < 0){
        if(errno != EINTR){
            ERROR_EXIT("Error while sem_wait", strerror(errno));
        }
    }
    if (buf->terminate) {
        exit(EXIT_SUCCESS);
    }
}

/**
 * @brief Informs generators that data has been read and can be overwritten
 * with new data.
 * @details Uses global variable sem_free.
 */
static void signal_read() {
    if(sem_post(sem_free) < 0){
        ERROR_EXIT("Error while sem_post", strerror(errno));
    }
}

/**
 * @brief Defines an exit function, creates and maps shared memory, sets
 * signal handler, initializes shared memory buffer, creates semaphores.
 * @details Sets global variables shm_fd, buf, sem_used, sem_free, sem_mutex.  
 */
static void initialize() {
    // exit cleanup function
    if(atexit(shutdown) < 0) {
        ERROR_EXIT("Error setting cleanup function", NULL);
    }

    // create shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm_fd < 0) {
        ERROR_EXIT("Error creating shared memory", strerror(errno));
    }

    // set size of shared memory
    if (ftruncate(shm_fd, sizeof(circular_buffer)) < 0) {
        ERROR_EXIT("Error setting size of shared memory", strerror(errno));
    }

    // map shared memory object
    buf = mmap(NULL, sizeof(*buf), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (buf == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory", strerror(errno));
    }
    if (close(shm_fd) < 0) {
        ERROR_MSG("Error closing shared memory fd", strerror(errno));
    }
    shm_fd = -1;

    // set signal handler
    struct sigaction sa = { .sa_handler = handle_signal };
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0) {
        ERROR_EXIT("Error setting signal handler", strerror(errno));
    }

    // initialize buffer
    buf -> terminate = 0;
    buf -> read_pos = 0;
    buf -> write_pos = 0;
    buf -> num_of_generators = 0;

    // create semaphores
    sem_used = sem_open(SEM_USED, O_CREAT | O_EXCL, 0600, 0);
    if (sem_used == SEM_FAILED) {
        ERROR_EXIT("Error creating used_sem", strerror(errno));
    }
    sem_free = sem_open(SEM_FREE, O_CREAT | O_EXCL, 0600, BUF_SIZE);
    if (sem_free == SEM_FAILED) {
        ERROR_EXIT("Error creating sem_free", strerror(errno));
    }
    sem_mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, 0600, 1);
    if (sem_mutex == SEM_FAILED) {
        ERROR_EXIT("Error creating sem_mutex", strerror(errno));
    }
}

/**
 * @brief Exit function. Unmaps, closes and unlinks shared memory, closes and unlinks semaphores.
 * @details Uses global variables shm_fd, buf, sem_used, sem_free, sem_mutex.  
 */
static void shutdown() {
    if (buf != NULL) {
        buf -> terminate = 1;

        // Stop all waiting generators from waiting
        if (sem_free != NULL) {
            for (size_t i = 0; i < buf->num_of_generators; i++) {
                if(sem_post(sem_free) < 0) {
                    ERROR_MSG("Error while sem_post for sem_free", strerror(errno));
                }
            }
        }
    }

    // Close shared memory fd (if not already closed)
    if (shm_fd != -1) {
        if (close(shm_fd) < 0) {
            ERROR_MSG("Error closing shared memory fd", strerror(errno));
        }
        shm_fd = -1;
    }

    // Close and unlink semaphores
    if (sem_used != NULL) {
        if (sem_close(sem_used) < 0) {
            ERROR_MSG("Error closing sem_used", strerror(errno));
        }
        if (sem_unlink(SEM_USED) < 0) {
            ERROR_MSG("Error unlinking SEM_USED", strerror(errno));
        }
    }
    if (sem_free != NULL) {
        if (sem_close(sem_free) < 0) {
            ERROR_MSG("Error closing sem_free", strerror(errno));
        }
        if (sem_unlink(SEM_FREE) < 0) {
            ERROR_MSG("Error unlinking SEM_FREE", strerror(errno));
        }
    }
    if (sem_mutex != NULL) {
        if (sem_close(sem_mutex) < 0) {
            ERROR_MSG("Error closing sem_mutex", strerror(errno));
        }
        if (sem_unlink(SEM_MUTEX) < 0) {
            ERROR_MSG("Error unlinking SEM_MUTEX", strerror(errno));
        }
    }

    // Unmap shared memory
    if (buf != NULL) {
        buf -> terminate = 1;
        if (munmap(buf, sizeof(*buf)) < 0) {
            ERROR_MSG("Error unmapping shared memory", strerror(errno));
        }
    }

    // Unlink shared memory
    if (shm_unlink(SHM_NAME) < 0) {
        ERROR_MSG("Error unlinking shared memory", strerror(errno));
    }
}

/**
 * @brief Informs generators that they should terminate and then exits.
 * @details Sets terminate-flag of shared buffer to inform generators that they 
 * should terminate. Additionally increments sem_free semaphore to prevent deadlocking 
 * of blocking generators.Uses global variables shm_fd, buf, sem_used, sem_free, sem_mutex.  
 */
static void handle_signal(int signal) {
    buf->terminate = 1;
}

static void ERROR_EXIT(char *message, char *error_details) {
    ERROR_MSG(message, error_details);
    exit(EXIT_FAILURE);
}

static void ERROR_MSG(char *message, char *error_details) {
    if (error_details == NULL) {
        fprintf(stderr, "[%s]: %s\n", PROGRAM_NAME, message);
    } else {
        fprintf(stderr, "[%s]: %s (%s)\n", PROGRAM_NAME, message, error_details);
    }
}

static void USAGE() {
    fprintf(stderr, "Usage: ./supervisor\n");
    exit(EXIT_FAILURE);
}