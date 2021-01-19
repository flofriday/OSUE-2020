#include "fb_arc_set.h"

static void generate_solutions(edge edges[]);
static void fill_vertex_array(vertex vertices[]);
static void generate_random_permutation(vertex vertices[]);
static unsigned int get_random_seed(void);
static void parse_input(int argc, const char** argv, edge edges[]);
static edge parse_edge(const char *arg);
static void write_buffer(edge_container candidate);
static void wait_write(void);
static void signal_write(void);
static void initialize(void);
static void shutdown(void);
static void ERROR_EXIT(char* message, char* error_details);
static void ERROR_MSG(char* message, char* error_details);
static void USAGE();
static const char* PROGRAM_NAME;

/**
 * @file generator.c
 * @author Michael Huber 11712763
 * @date 12.11.2020
 * @brief OSUE Exercise 1B fb_arc_set
 * @details The generator program takes a graph as input. 
 * The program repeatedly generates a random solution to 
 * the problem as described on the first page and writes 
 * its result to the circular buffer. It repeats this 
 * procedure until it is notified by the supervisor 
 * to terminate.
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

/** Number of edges of input graph */
static size_t num_of_edges;
/** Number of vertices of input graph */
static size_t num_of_vertices;

/**
 * @brief Generates solutions based on input graph, writes them to shared memory buffer.
 * @details Uses the global variables shmfd, buf, used_sem, free_sem, mutex_sem.
 */
int main(int argc, const char** argv) {
    PROGRAM_NAME = argv[0];

    // initialize resources
    initialize();

    // parse input
    num_of_edges = argc-1;
    edge edges[num_of_edges];
    parse_input(argc, argv, edges);

    // generate solution
    srand(get_random_seed());
    generate_solutions(edges);

    exit(EXIT_SUCCESS);
}

/**
 * @brief Generates solutions and writes them to shared memory buffer.
 * @param edges Array of edges of graph to generate solutions for
 * @details Calls write_buffer; uses global variables buf.
 * @see generate_random_permutation, write_buffer
 */
static void generate_solutions(edge edges[]) {
    while (buf->terminate == 0) {
        vertex random_permutation[num_of_vertices];
        fill_vertex_array(random_permutation);
        generate_random_permutation(random_permutation);
        
        edge_container to_delete;

        to_delete.counter = 0;
        size_t delete_counter = 0;
        for (size_t i = 0; i < num_of_edges && delete_counter < 7; i++)
        {
            size_t pos_u = random_permutation[edges[i].u];
            size_t pos_v = random_permutation[edges[i].v];
            if(pos_u > pos_v) {
                to_delete.container[delete_counter++] = edges[i];
                to_delete.counter = delete_counter;
            }
        }
        if (delete_counter >= 7) {
            continue;
        } else {
            write_buffer(to_delete);
        }
    }
}

/**
 * @brief Fills the given vertex array with the numbers of 0 to n-1 (n being the size of the array).
 * @param vertices Array to be filled
 */
static void fill_vertex_array(vertex vertices[]) {
    for (size_t i = 0; i < num_of_vertices; i++) {
        vertices[i] = i;
    }
}

/**
 * @brief Generates a random permutation of the given vertices.
 * @param edges Array to be filled with random permutation
 * @details This is done using the Fisher-Yates shuffle algorithm.
 */
static void generate_random_permutation(vertex vertices[]) {
    for (size_t i = num_of_vertices-1; i > 0; i--)
    {
        vertex j = rand() % (i+1);
        vertex temp = vertices[j];
        vertices[j] = vertices[i];
        vertices[i] = temp;
    }
}

/**
 * @brief Returns a random seed for usage in srand()
 * @details This is achieved by multiplying time(2),
 * clock(3) and getpid(2).
 */
static unsigned int get_random_seed() {
    return time(NULL) * clock() * getpid();
}


/**
 * @brief Parses a graph from the input args and fills edges array with it.
 * @param argc Argument counter from main function
 * @param argv Argument array from main function
 * @param edges Array to be filled with parsed edges
 * @details Additionally, it sets the global variable num_of_vertices 
 * correctly. Exits with an error if parsing goes wrong.
 */
static void parse_input(int argc, const char** argv, edge edges[]) {
    // exit if no edges given
    if(argc == 1){
        USAGE();
    }

    // parse input
    for (size_t i = 1; i < argc; i++){
        edges[i-1] = parse_edge(argv[i]);
    }
}

/**
 * @brief Parses an edge in the form "u-v" from an argument.
 * @param arg Argument to parse edge from
 * @return Parsed edge
 * @details This is done using strtol(3). This function allocates 
 * memory for a string but frees it itself upon error or completion.
 */
static edge parse_edge(const char* arg) {
        char *input = strdup(arg);
        if (input == NULL) {
            ERROR_EXIT("Error duplicating argument string", strerror(errno));
        }
        
        // parse first vertex
        char *endptr;
        char *vertex1 = input;
        long u = strtol(vertex1, &endptr, 0);
        if (endptr == vertex1) {
            fprintf(stderr, "[%s]: Invalid vertex index ('%s' is not a number)\n", PROGRAM_NAME, vertex1);
            free(input);
            USAGE();
        }
        if (u == LONG_MIN || u == LONG_MAX) {
            free(input);
            ERROR_EXIT("Overflow occurred while parsing vertex index", strerror(errno));
        }
        if (endptr[0] != '-') {
            fprintf(stderr, "[%s]: Invalid vertex delimiter '%c' (has to be '-')\n", PROGRAM_NAME, endptr[0]);
            free(input);
            USAGE();
        }
        if (u < 0) {
            fprintf(stderr, "[%s]: Negative vertex index %ld not allowed\n", PROGRAM_NAME, u);
            free(input);
            USAGE();
        }

        // parse second vertex
        char *vertex2 = endptr+1;
        long v = strtol(vertex2, &endptr, 0);
        if (endptr == vertex2) {
            fprintf(stderr, "[%s]: Invalid vertex index ('%s' is not a number)\n", PROGRAM_NAME, vertex2);
            free(input);
            USAGE();
        }
        if (v == LONG_MIN || v == LONG_MAX) {
            free(input);
            ERROR_EXIT("Overflow occurred while parsing vertex index", strerror(errno));
        }
        if (endptr[0] != '\0') {
            fprintf(stderr, "[%s]: Invalid edge delimiter '%c' (has to be ' ')\n", PROGRAM_NAME, endptr[0]);
            free(input);
            USAGE();
        }
        if (v < 0) {
            fprintf(stderr, "[%s]: Negative vertex index %ld not allowed\n", PROGRAM_NAME, v);
            free(input);
            USAGE();
        }

        free(input);

        // update number of vertices
        if (u+1 > num_of_vertices) {
            num_of_vertices = u+1;
        }
        if (v+1 > num_of_vertices) {
            num_of_vertices = v+1;
        }

        // return edge
        edge e = {.u = u, .v = v};
        return e;
}

/**
 * @brief Writes new solution to the circular buffer. Blocks
 * until there is space to write solution
 * @param edge_container containing a solution candidate
 * @details Calls wait_write, signal_write; uses global variables buf.
 * @see wait_write, signal_write
 */
static void write_buffer(edge_container candidate) {
    wait_write();
    buf->data[buf->write_pos] = candidate;
    buf->write_pos = (buf->write_pos + 1) % BUF_SIZE;
    signal_write();
}

/**
 * @brief Blocks until there is space in the buffer to write and
 * until mutual exclusion is guaranteed.
 * @details Uses global variable sem_free and sem_mutex.
 */
static void wait_write() {
    if (sem_wait(sem_free) < 0) {
        if (errno == EINTR) {
            exit(EXIT_SUCCESS);
        }
        ERROR_EXIT("Error while waiting for sem_free", strerror(errno));
    }
    if (buf->terminate) {
        exit(EXIT_SUCCESS);
    }
    if (sem_wait(sem_mutex)) {
        if (errno == EINTR) {
            exit(EXIT_SUCCESS);
        }
        ERROR_EXIT("Error while waiting for sem_mutex", strerror(errno));
    }
}

/**
 * @brief Informs generators that mutual exclusion section was left and
 * informs supervisors that there is new data to be read.
 * @details Uses global variable sem_mutex and sem_used.
 */
static void signal_write() {
    if (sem_post(sem_mutex) < 0) {
        ERROR_EXIT("Error while posting sem_mutex", strerror(errno));
    }
    if (sem_post(sem_used) < 0) {
        ERROR_EXIT("Error while posting sem_used", strerror(errno));
    }
}

/**
 * @brief Defines an exit function, opens and maps shared memory, opens semaphores.
 * @details Sets global variables shm_fd, buf, sem_used, sem_free, sem_mutex.  
 */
static void initialize() {
    // exit cleanup function
    if(atexit(shutdown) < 0) {
        ERROR_EXIT("Error setting cleanup function", NULL);
    }

    // open shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shm_fd < 0) {
        if (errno == ENOENT) {
            ERROR_MSG("Supervisor has to be started first!", NULL);
        }
        ERROR_EXIT("Error opening shared memory", strerror(errno));
    }

    // map shared memory object
    buf = mmap(NULL, sizeof(*buf), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (buf == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory", strerror(errno));
    }
    if (close(shm_fd) < 0) {
        ERROR_EXIT("Error closing shared memory fd", strerror(errno));
    }
    shm_fd = -1;

    // open semaphores
    sem_used = sem_open(SEM_USED, 0);
    if (sem_used == SEM_FAILED) {
        ERROR_EXIT("Error opening used_sem", strerror(errno));
    }
    sem_free = sem_open(SEM_FREE, 0);
    if (sem_free == SEM_FAILED) {
        ERROR_EXIT("Error opening sem_free", strerror(errno));
    }
    sem_mutex = sem_open(SEM_MUTEX, 0);
    if (sem_mutex == SEM_FAILED) {
        ERROR_EXIT("Error opening sem_mutex", strerror(errno));
    }

    // increment generator counter in shm
    buf->num_of_generators++;
}

/**
 * @brief Exit function. Unmaps and closes shared memory, closes semaphores.
 * @details Uses global variables shm_fd, buf, sem_used, sem_free, sem_mutex.  
 */
static void shutdown() {
    if (buf != NULL) {
        buf->num_of_generators--;
        if (munmap(buf, sizeof(*buf)) < 0) {
            ERROR_MSG("Error unmapping shared memory", strerror(errno));
        }
    }

    if (shm_fd != -1) {
        if (close(shm_fd) < 0) {
            ERROR_MSG("Error closing shared memory fd", strerror(errno));
        }
    }

    if (sem_used != NULL) {
        if (sem_close(sem_used) < 0) {
            ERROR_MSG("Error closing sem_used", strerror(errno));
        }
    }

    if (sem_free != NULL) {
        if (sem_close(sem_free) < 0) {
            ERROR_MSG("Error closing sem_free", strerror(errno));
        }
    }

    if (sem_mutex != NULL) {
        if (sem_post(sem_mutex) < 0) { // Resolve possible deadlocks
            ERROR_MSG("Error while sem_post on sem_free", strerror(errno));
        }
        if (sem_close(sem_mutex) < 0) {
            ERROR_MSG("Error closing sem_mutex", strerror(errno));
        }
    }
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
    fprintf(stderr, "Usage: %s EDGE1 EDGE2 ...\n", PROGRAM_NAME);
    fprintf(stderr, "Example: %s 0-1 1-2 1-3 1-4 2-4 3-6 4-3 4-5 6-0\n", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}