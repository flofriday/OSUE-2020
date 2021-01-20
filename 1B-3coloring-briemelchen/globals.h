/**
 * @file globals.h
 * @author briemelchen
 * @brief globals.h defines global used structs and constant-macros
 * @details module describes all global macros e.g semaphore and shared mem names as well as size's for the buffer.
 * Structs which are used globally are also defined (edge, graph, solution)
 * 
 * last modified: 14.11.2020
 */

#ifndef globals_h
#define globals_h
#define SHM_NAME "/11xxxxxx_shm"
#define SEM_NAME_WR "/11xxxxxx_sem_wr" // free space sem
#define SEM_NAME_RD "/11xxxxxx_sem_rd" // used space sem
#define SEM_NAME_WR_BLOCK "/11828347_sem_blocked_wr" // blocking sem
#define BUFFER_LENGTH 16 //if buffer should hold more possible solution change here accordingly
#define ACCEPTED_SOL 8   //if a solution with more edges is ok (e.g for large graphs) change accordingly. ATTENTION: Buffer size grows with growing solution size
#define EDGE_REGEX_PATTERN "[0-9]+-[0-9]+$" //regex pattern for valid edge inputs

typedef  int node;
typedef struct edge
{
    node start_node;
    node end_node;
} edge;

typedef struct graph
{
    int node_c;
    int edge_c;
    edge *edges;
} graph;

typedef struct solution
{
    edge edges[ACCEPTED_SOL]; // solutions are only possible, if they are in the accepted size, therefore static array is used
    int removed_edges;
    int origin_edge_count;
} solution;

typedef struct circ_buffer
{
    solution sol[BUFFER_LENGTH];
    volatile int write_pos;
    volatile int read_pos;
    volatile int sig_state;
} circ_buffer;

#endif