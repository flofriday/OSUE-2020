/**
 * @file solver.h
 * @author briemelchen
 * @brief defines functions which are used to compute solutions of the 3-color-problem.
 * Util functions (printing,parsinge etc.) are provided aswell.
 * @details Calculates  randomized solutions to the 3 color problem to a specific graph.
 */

#include "buffer.h"
#ifndef solver_h
#define solver_h

/**
 * @brief Computes a valid 3-coloring of a specific graph.
 * @details Computation is performed randomized: Nodes are colored randomly, and afterwards
 * edges with same start and end node color are removed, and therefore a valid 3-coloring is computed.
 * Only solutions with a max-edge-count of ACCEPTED_SOL (see globals.h) are returned.
 * @param graph on which the solution should be computed
 * @return the solution of the 3-coloring. If the solution.removed_edges == -1 a solution is invalid and should be discarded by a generator.
 */
solution calculate_solution(graph *graph);

/**
 * @brief parses a given graph and saves it as edge structs.
 * @details graph must be handed in format d-d where d is an integer.
 * @param argv Argument vector holding the graph.
 * @param edges where the graph should be stored in.
 * @return 0 on success, -1 on failure.
 */
int parse_graph(char **argv, edge *edges);

/**
 * @brief prints a given graph to stdout.
 * @details count of edges and nodes are printed as well as a visual representation of each edge: d-d
 * where d is an integer.
 * @param g graph which should be printed.
 */
void print_graph(graph *g);

/**
 * @brief prints a solution to stdout.
 * @details prints the solution in following format:
 * - if the graph is 3-colorable: The graph is 3-colorable! will be printed
 * - otherwise the count of the edges and a visual representation of the edges will be printed.
 * @param solution  to be printed
 */
void print_solution(solution solution);

/**
 * @brief gets the amount of nodes the graph contains
 * @details gets it by comparing the edges, because highest node of an edge must be the amount of nodes.
 * @param graph pointer to a graph which nodes should be counted
 */
void get_node_c(graph *graph);

#endif
