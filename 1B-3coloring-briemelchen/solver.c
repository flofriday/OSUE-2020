/**
 * @file solver.c
 * @author briemelchen
 * @date 03.11.2020
 * @brief Implementation of solver.h. defines functions which are used to compute solutions of the 3-color-problem.
 * Util functions (Printing,Parsinge etc.) are provided as well.
 * @details Calculates  randomized solutions to the 3 color problem to a specific graph.
 * 
 * last modified: 14.11.2020
 */

#include "solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <limits.h>

/**
 * @brief compares a string to match a regex.
 * @details regex which is used to match is defined in globals.h
 * @param str to be checked
 * @return 0  if it matches the regular expression, otherwise false
 */
static int reg_matches(const char *str);

solution calculate_solution(graph *graph)
{
    solution solution;
    solution.removed_edges = 0;
    solution.origin_edge_count = graph->edge_c;
    // sets all edges of the solution to invalid edges
    for (int i = 0; i < ACCEPTED_SOL; i++)
    {
        solution.edges[i].start_node = INT_MIN;
        solution.edges[i].end_node = INT_MIN;
    }

    // holds the colors of each node of the graph.
    // The index of the array is the label of the node (e.g colored_nodes[0]=1)Node with label 0 has color 1
    int colored_nodes[graph->node_c];
    for (int i = 0; i < graph->node_c; i++)
    {   
     
        colored_nodes[i] = (rand() % 3); // seed has to be set by the generator
      //  printf("Node: %d has color: %d\n", i, colored_nodes[i]);
    }
    int ac_edge_in_sol = 0;
    for (int i = 0; i < graph->edge_c && ac_edge_in_sol < ACCEPTED_SOL; i++)
    {
        if (colored_nodes[graph->edges[i].start_node] ==
            colored_nodes[graph->edges[i].end_node]) // edge not in 3-color-solution and therefore in a solution
        {
            solution.edges[ac_edge_in_sol].start_node = graph->edges[i].start_node;
            solution.edges[ac_edge_in_sol].end_node = graph->edges[i].end_node;
            ac_edge_in_sol++;
            solution.removed_edges++;
        }
        if (solution.removed_edges >= ACCEPTED_SOL)
        {
            solution.removed_edges = -1;
            
        }
    }

   
    return solution;
}

int parse_graph(char **argv, edge *edges)
{
    for (int i = 1; argv[i] != NULL; i++)
    {
        if (reg_matches(argv[i]) != 0)
        {
            return -1;
        }

        char *parts = strtok(argv[i], "-");
        edge edge;
        edge.start_node = strtol(parts, NULL, 10); // converts string to integer
        parts = strtok(NULL, "-");
        edge.end_node = strtol(parts, NULL, 10);
        edges[i - 1] = edge; // i-1  because loop starts at 1
    }

    return 0;
}

void print_graph(graph *g)
{
    printf("Node-Count: %d\n", g->node_c);
    printf("Edge-Count: %d\n", g->edge_c);
    for (size_t i = 0; i < g->edge_c; i++)
    {
        printf("Edge: %d-%d\n", g->edges[i].start_node, g->edges[i].end_node);
    }
}

void print_solution(solution solution)
{
    if (solution.removed_edges == 0)
    {
        printf("The graph is 3-colorable!\n");
        return;
    }
    int removed = solution.removed_edges;
    printf("Solution with %d edges: ", removed);
  //  printf("Origin edge count: %d",solution->origin_edge_count);
    for (int i = 0; i < removed; i++)
    {   
        int start = solution.edges[i].start_node;
        int end = solution.edges[i].end_node;
        printf("%d-%d ", start, end);
    }
    printf("\n");
}

void get_node_c(graph *graph)
{
    int max = -1;
    for (size_t i = 0; i < graph->edge_c; i++)
    {
        if (graph->edges[i].end_node > max)
            max = graph->edges[i].end_node;
        if (graph->edges[i].start_node > max)
            max = graph->edges[i].start_node;
    }

    graph->node_c = max + 1;
}

static int reg_matches(const char *str)
{
    regex_t regex;
    int return_val;
    if (regcomp(&regex, EDGE_REGEX_PATTERN, REG_EXTENDED) != 0)
    {
        return -1;
    }
    return_val = regexec(&regex, str, (size_t)0, NULL, 0);
    regfree(&regex);
    return return_val == 0 ? 0 : -1;
}