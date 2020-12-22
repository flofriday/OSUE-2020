/**
 * @project: Integer Multiplication
 * @module intmul
 * @author Johannes Zottele 11911133
 * @version 1.0
 * @date 17.12.2020
 * @section File Overview
 * Treerep includes all functions to manage the treerepresentation of the child processes of a process
 */

/**
 * @brief Creates a string of the form "intmul(nr1, nr2)"... uses malloc for the created string
 */
void process_to_string(char **str, char *nr1, char *nr2);

/**
 * @brief Reads results from child through the given pipe line by line and creates the new result of this processes. It prints the result to stdout.
 */
void read_and_print(int pipefd[4][2], char *pname);