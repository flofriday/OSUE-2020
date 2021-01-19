#include "intmul.h"

// pipes
static void create_pipes(void);
static void close_pipes(void);
static void close_unused_pipes(void);

// utils
static int hextoint(char c);
static char inttohex(int n);
static void cleanup(void);
static const char* PROGRAM_NAME;

// main
static size_t readinput(void);
static void base_case(char cA, char cB);
static void invoke_child(int i);
static void write_to_children(int len);
static void wait_for_children(void);
static void read_from_children_and_print_combined_result(int len);

// error handling
static void ERROR_EXIT(char* message, char* error_details);
static void ERROR_MSG(char* message, char* error_details);
static void USAGE(void);

/**
 * @file intmul.c
 * @author Michael Huber 11712763
 * @date 20.12.2020
 * @brief OSUE Exercise 2 intmul
 * @details The program takes two hexadecimal integers A and B with an equal number of digits as 
 * input, multiplies them and prints the result. The input is read from stdin and consists of two 
 * lines: the first line is the integer A and the second line is the integer B.
 */

/** Struct for storing input numbers */
static intmul_input_t numbers;

/** Array for storing information about children (pid, pipes) */
static child_process_t children[NUM_CHILDREN];

/** Array for storing all pipe ends (to be able to close them in a loop) */
static pipe_end_t pipe_ends[NUM_PIPE_ENDS];

/** Booleans set by options */
static bool tree, parent;

/**
 * @brief Sets up cleanup function, parses options, calls functions to fork children and
 * process the results. 
 * @details Uses global variables tree, parent, numbers, children.
 * Calls readinput, base_case, create_pipes, invoke_child, write_to_children, 
 * close_unused_pipes, wait_for_children, read_from_children_and_print_combined_result.
 */
int main(int argc, char* const *argv) {
    PROGRAM_NAME = argv[0];

    // exit cleanup function
    if(atexit(cleanup) < 0) {
        ERROR_EXIT("Error setting cleanup function", NULL);
    }

    // parse options
    int count_t = 0;
    int c;  
    while((c = getopt(argc, argv, "tT")) != -1 ) {
        switch (c) {
            case 't':
                parent = true;
                tree = true;
                count_t++;
                break;
            case 'T':
                tree = true;
                count_t++;
                break;
            case '?':
                USAGE();
                break;
            default:
                USAGE();
                break;
        }
    }

    // Wrong usage
    if (count_t > 1 || argc > 2 || (argc > 1 && count_t > 1)) {
        USAGE();
    }

    // initialize input struct
    numbers.A = NULL;
    numbers.B = NULL;

    // read in number
    size_t len = readinput(); // freeing is done in cleanup function

    // recursion base case
    if (len == 1) {
        base_case(numbers.A[0], numbers.B[0]);
    } 
    
    // fork 4 children
    create_pipes();
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        pid_t pid = fork();
        if (pid == -1) {
            ERROR_EXIT("Cannot fork", strerror(errno));
        }
        if (pid == 0) {
            invoke_child(i);
        }
        children[i].pid = pid;
    }

    // close pipes not used by the parent
    close_unused_pipes();

    // write Ah, Al, Bh, Bl to each child
    write_to_children(len);

    // read the result from child, add the results together and print it to stdout
    read_from_children_and_print_combined_result(len);

    // check exit status of children and abort if failure
    wait_for_children();

    // used pipes are implicitly closed in write and read functions

    exit(EXIT_SUCCESS);
}

/**
 * @brief Reads the first two lines of stdin, validates if they are valid hex strings, have even length
 * and puts them into the numbers struct
 * @details Fills the global struct numbers
 */
static size_t readinput(void){

    numbers.A = NULL;
    size_t len1, buflen1;
    if((len1 = getline(&(numbers.A), &buflen1, stdin)) < 0) {
        ERROR_EXIT("Reading of first line failed", strerror(errno));
    }

    numbers.B = NULL;
    size_t len2, buflen2;
    if((len2 = getline(&(numbers.B), &buflen2, stdin)) < 0) {
        ERROR_EXIT("Reading of second line failed", strerror(errno));
    }

    if (numbers.A[len1-1] == '\n') {
        numbers.A[len1-1] = '\0';
        len1 = len1-1;
    }

    if (numbers.B[len2-1] == '\n') {
        numbers.B[len2-1] = '\0';
        len2 = len2-1;
    }

    if (len1 != len2) {
        ERROR_EXIT("Numbers do not have equal length", NULL);
    }

    size_t len = len1;

    if (len == 0) {
        ERROR_EXIT("Input is empty", NULL);
    }

    if ((len & (len-1)) != 0) {
        ERROR_EXIT("Input length is not a power of two", NULL);
    }

    for (size_t i = 0; i < len; i++)
    {
        if(isxdigit(numbers.A[i]) == false) {
            char errormsg[64];
            snprintf(errormsg, 64, "Digit %c (value %d) of line 1 is no hex digit", numbers.A[i], (unsigned int)numbers.A[i]);
            ERROR_EXIT(errormsg, NULL);
        }
        if(isxdigit(numbers.B[i]) == false) {
            char errormsg[64];
            snprintf(errormsg, 64, "Digit %c (value %d) of line 2 is no hex digit", numbers.B[i], (unsigned int)numbers.B[i]);
            ERROR_EXIT(errormsg, NULL);
        }
    }

    return len;
}

/**
 * @brief Recursion base case - multiplies two one-digit hex numbers and prints the result to stdout
 * @param cA First hex char
 * @param cB Second hex char
 */
static void base_case(char cA, char cB) {
    int A = hextoint(cA);
    int B = hextoint(cB);
    int result = A * B;

    fprintf(stdout, "%02x\n", result);
    if (tree) {
        int padwidth = (TREE_COLUMN_WIDTH/4-11);
        int padding1 = padwidth/2;
        int padding2 = padwidth % 2 == 0 ? padwidth/2 : padwidth/2 + 1;
        fprintf(stdout, "%*sINTMUL(%c,%c)%*s$", padding1, " ", cA, cB, padding2, " ");
        fflush(stdout);
    }
    exit(EXIT_SUCCESS);
}

/**
 * @brief Creates pipes for communication with child processes
 * @details Fills the global arrays children and pipe_ends
 */ 
static void create_pipes(void) {
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        int par_to_child[2], child_to_par[2];

        if(pipe(par_to_child) + pipe(child_to_par) < 0) {
            ERROR_EXIT("Error while creating pipes", strerror(errno));
        }

        children[i].pipes[IN].ends[WRITE] = par_to_child[WRITE];
        children[i].pipes[IN].ends[READ] = par_to_child[READ];
        children[i].pipes[OUT].ends[WRITE] = child_to_par[WRITE];
        children[i].pipes[OUT].ends[READ] = child_to_par[READ];

        pipe_ends[4*i] = par_to_child[READ];
        pipe_ends[4*i+1] = par_to_child[WRITE];
        pipe_ends[4*i+2] = child_to_par[READ];
        pipe_ends[4*i+3] = child_to_par[WRITE];
    }
}

/**
 * @brief Executed by a child after forking - redirects pipe ends, closes pipes and calls program
 * recursively
 * @param i Index of child to invoke
 * @details Uses global array children to access pipes 
 */ 
static void invoke_child(int i) {
    // redirect stdin to pipe
    if((dup2(children[i].pipes[IN].ends[READ], STDIN_FILENO)) < 0) {
        ERROR_EXIT("Child AhBh: Error while redirecting stdin to pipe", strerror(errno));
    }

    // redirect stdout to pipe
    if((dup2(children[i].pipes[OUT].ends[WRITE], STDOUT_FILENO)) < 0) {
        ERROR_EXIT("Child AhBh: Error while redirecting stdout to pipe", strerror(errno));
    }

    // close all pipes in child process
    close_pipes();

    // recursive call
    char* option = NULL;
    if (tree) option = "-T";
    if(execlp(PROGRAM_NAME, PROGRAM_NAME, option, NULL) < 0){
        ERROR_EXIT("Error while execlp child AhBh", strerror(errno));
    }
}

/** 
 * @brief Closes all pipe ends
 * @details Uses global array pipe_ends to access pipes
 */
static void close_pipes(void) {
    for (size_t i = 0; i < NUM_PIPE_ENDS; i++)
    {
        if(close(pipe_ends[i]) < 0) {
            ERROR_EXIT("Error while closing pipe", strerror(errno));
        }
    }
    
}

/** 
 * @brief Closes pipes not used by the parent
 * @details Uses the global array children to access pipes
 */
static void close_unused_pipes() {
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        if (close(children[i].pipes[IN].ends[READ]) + close(children[i].pipes[OUT].ends[WRITE]) < 0) {
            ERROR_EXIT("Error while closing unused pipe ends", strerror(errno));
        }
    }
}

/**
 * @brief Splits the input lines in half and feeds it to the corresponding children
 * @details Uses global array children to access pipes of children and write to them
 * @param len Length of parent input
 */
static void write_to_children(int len) { 
    // open files for writing to children
    FILE* in[NUM_CHILDREN];
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        in[i] = fdopen(children[i].pipes[IN].ends[WRITE], "w");
    }

    int half_len = len/2;

    char* Ah = numbers.A;
    char* Al = numbers.A + half_len;
    char* Bh = numbers.B;
    char* Bl = numbers.B + half_len;

    // write parts to each child
    for (size_t i = 0; i < NUM_CHILDREN; i++) {
        if (i == AhBh || i == AhBl) fprintf(in[i], "%.*s\n", half_len, Ah);
        if (i == AlBh || i == AlBl) fprintf(in[i], "%.*s\n", half_len, Al);
        if (i == AhBh || i == AlBh) fprintf(in[i], "%.*s\n", half_len, Bh);
        if (i == AhBl || i == AlBl) fprintf(in[i], "%.*s\n", half_len, Bl);
    }
    
    // close input files
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        if (fclose(in[i]) < 0) {
            ERROR_MSG("Error while closing input files", strerror(errno));
        }
    }
}

/**
 * @brief Waits for termination of each child
 * @details Uses global array children to access pids
 */ 
static void wait_for_children() {
    // wait for children
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        int status;
        if (waitpid(children[i].pid, &status, 0) < 0) {
            ERROR_EXIT("Error while waiting for children", strerror(errno));
        }

        if (WEXITSTATUS(status) != EXIT_SUCCESS) {
            ERROR_EXIT("Child exited with error", NULL);
        }
    }
}

/** 
 * @brief Reads output of children, combines the results and prints them to stdout
 * @details Uses global array children to access pipes for reading from children
 * @param len Length of parent input
 */ 
static void read_from_children_and_print_combined_result(int len) {
    // open files for reading from childs
    FILE* out[NUM_CHILDREN];
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        out[i] = fdopen(children[i].pipes[OUT].ends[READ], "r");
    }

    // read each child result
    char* child_results[NUM_CHILDREN];
    ssize_t child_results_len[NUM_CHILDREN];
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        child_results[i] = NULL;
        ssize_t result_len = 0;
        size_t buflen = 0;
        if((result_len = getline(&(child_results[i]), &buflen, out[i])) < 0) {
            ERROR_MSG("Reading of children result failed", strerror(errno));
        }
        if (child_results[i][result_len-1] == '\n') {
            child_results[i][result_len-1] = '\0';
            result_len--;
        }
        child_results_len[i] = result_len;
    }

    // combine children results
    int half_len = len/2;
    int double_len = 2*len;

    // the result can have at most 2*input-length digits
    char result[double_len+1];
    memset(result, 0, double_len);
    result[double_len] = '\0';

    int index[NUM_CHILDREN];
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        index[i] = child_results_len[i]-1;
    }

    int carry = 0;
    for (int i = double_len-1; i >= 0; i--)
    {
        if(i >= len+half_len) {
        // Al*Bl 
        // (for first n/2 digits can be just copied)
            result[i] = index[AlBl] < 0 ? 0 : child_results[AlBl][index[AlBl]--];
        } else if (i >= len) {
        // Ah*Bl * 16^n/2 + Al*Bh * 16^n/2 + Al*Bl 
        // (for next n/2 digits AhBl and AlBh need to be taken into account)
            int n0 = index[AlBl] < 0 ? 0 : hextoint(child_results[AlBl][index[AlBl]--]);
            int n1 = index[AlBh] < 0 ? 0 : hextoint(child_results[AlBh][index[AlBh]--]);
            int n2 = index[AhBl] < 0 ? 0 : hextoint(child_results[AhBl][index[AhBl]--]);

            int n = n0 + n1 + n2 + carry;
            
            result[i] = inttohex(n % 16);
            carry = n / 16;
        } else {
        // Ah*Bh * 16^n + Ah*Bl * 16^n/2 + Al*Bh * 16^n/2 + Al*Bl
        // (for the remaining digits all subresults have to be combined)
            int n0 = index[AlBl] < 0 ? 0 : hextoint(child_results[AlBl][index[AlBl]--]);
            int n1 = index[AlBh] < 0 ? 0 : hextoint(child_results[AlBh][index[AlBh]--]);
            int n2 = index[AhBl] < 0 ? 0 : hextoint(child_results[AhBl][index[AhBl]--]);
            int n3 = index[AhBh] < 0 ? 0 : hextoint(child_results[AhBh][index[AhBh]--]);

            int n = n0 + n1 + n2 + n3 + carry;
            
            result[i] = inttohex(n % 16);
            carry = n / 16;
        }
    }

    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        free(child_results[i]);
    }

    // print result to stdout
    fprintf(stdout, "%s\n", result);

    /* BONUS{ */
    if (tree) {
        // recursion depth
        int n = (int)log2((double)len);

        // rows and columns of current box
        int num_rows = 1 + 2*n;
        int num_columns = (int)pow(4, n-1);

        // total width of box in chars
        int width = num_columns * TREE_COLUMN_WIDTH;

        // read subtrees of children
        char* child_trees[NUM_CHILDREN];
        for (size_t i = 0; i < NUM_CHILDREN; i++)
        {
            child_trees[i] = NULL;
            ssize_t tree_len = 0;
            size_t buflen = 0;
            if((tree_len = getdelim(&(child_trees[i]), &buflen, '$', out[i])) < 0) {
                ERROR_EXIT("Reading of children tree failed", strerror(errno));
            }
            if (child_trees[i][tree_len-1] == '$') {
                child_trees[i][tree_len-1] = ' ';
                tree_len--;
            }
        }

        // generate centered parent node
        int parent_node_len = 9+2*len+1;
        char parent_node[parent_node_len];
        parent_node[parent_node_len] = '\0';
        snprintf(parent_node, parent_node_len, "INTMUL(%s,%s)", numbers.A, numbers.B);
        int parent_pad_len = (width - parent_node_len) / 2;
        printf("%*s%s%*s", parent_pad_len, "", parent_node, parent_pad_len, "");
        if (parent) printf("\n");

        // print edges
        printf("%*s%*s%*s%*s%*s", (width/8), "/", (width/4), "/", (width/4), "\\", (width/4), "\\", (width/8), " ");
        if (parent) printf("\n");

        // combine subtrees
        int child_width = width / 4;
        for (size_t i = 0; i < num_rows-2; i++)
        {
            printf("%.*s%.*s%.*s%.*s", 
                child_width, &(child_trees[0][0])+child_width*i, 
                child_width, &(child_trees[1][0])+child_width*i, 
                child_width, &(child_trees[2][0])+child_width*i, 
                child_width, &(child_trees[3][0])+child_width*i);
            if (parent) printf("\n");
        }

        if (!parent) printf("$");
        fflush(stdout);

        for (size_t i = 0; i < NUM_CHILDREN; i++)
        {
            free(child_trees[i]);
        }
    }
    /* }BONUS */
    
    for (size_t i = 0; i < NUM_CHILDREN; i++)
    {
        if(fclose(out[i]) < 0) {
            ERROR_EXIT("Error while closing output file", strerror(errno));
        }
    }
}

/**
 * @brief Frees the allocated input numbers (if not already freed)
 * @details Uses the global array numbers to free content
 */ 
static void cleanup(void) {
    // free numbers
    if (numbers.A != NULL) {
        free(numbers.A);
    }
    if (numbers.B != NULL) {
        free(numbers.B);
    }
}

/**
 * @brief Converts a hex char to the corresponding decimal integer
 * @param c Hex char to be converted
 */
static int hextoint(char c){
    int value;
    if (c >= 'A') {
        if (c >= 'a') {
            value = (c - 'a' + 10);
        } else {
            value = (c - 'A' + 10);
        }
    } else {
        value = (c - '0');
    }
    return value;
}

/**
 * @brief Converts an int 0<=n<=15 to the corresponding hex char
 * @param n Integer to be converted
 */
static char inttohex(int n) {
    char c;
    if (n>15 || n<0) {
        ERROR_MSG("invalid hex number", NULL);
        return -1;
    } else if (n<10) {
        c = '0'+n;
    } else {
        c = 'a'+(n-10);
    }
    return c;
}

static void ERROR_EXIT(char *message, char *error_details) {
    ERROR_MSG(message, error_details);
    exit(EXIT_FAILURE);
}

static void ERROR_MSG(char *message, char *error_details) {
    if (error_details == NULL) {
        fprintf(stderr, "[%s](%d): %s\n", PROGRAM_NAME, getpid(), message);
    } else {
        fprintf(stderr, "[%s](%d): %s (%s)\n", PROGRAM_NAME, getpid(), message, error_details);
    }
}

void USAGE(void) {
    fprintf(stderr, "Usage: %s [-t]", PROGRAM_NAME);
    exit(EXIT_FAILURE);
}