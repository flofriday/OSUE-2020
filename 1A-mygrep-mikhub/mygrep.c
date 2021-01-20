#include "mygrep.h"

static char* PROGRAM_NAME;
static void mygrep(char* keyword, FILE* in, FILE* out, bool case_sensitivity);
static bool strcontains(char* line, char* keyword, bool case_sensitivity);
static void ERROR_EXIT(char* message);
static void USAGE(void);

/**
 * @brief Reduced variation of the Unix-command grep. Reads in several files and 
 * prints all lines containing a keyword.
 * @author Michael Huber 11712763
 * @date 10.11.20
 * @brief OSUE Exercise 1A mygrep
 */
int main(int argc, char **argv) {
    PROGRAM_NAME = argv[0];

    /* READ OPTIONS */
    bool case_sensitivity = true;
    char* outfile = NULL;
    int count_i = 0, count_o = 0;
    
    int c;  
    while((c = getopt(argc, argv, "io:")) != -1 ) {
        switch (c) {
            case 'i':
                case_sensitivity = false;
                count_i++;
                break;

            case 'o':
                outfile = optarg;
                count_o++;
                break;

            case '?':
                USAGE();
                break;

            default:
                USAGE();
                break;
        }
    }

    // Options used more than once - wrong usage
    if (count_i > 1 || count_o > 1) {
        USAGE();
    }

    // Setup output
    FILE* out = stdout;
    if (outfile != NULL) {
        out = fopen(outfile, "w");
        if (out == NULL) {
            ERROR_EXIT(strerror(errno));
        }   
    }

    /* READ ARGUMENTS */
    int argind = optind;
    if (argind >= argc || argc < 2) {
        USAGE();
    }

    // Get keyword from args
    char* keyword = argv[argind];
    argind++;
    
    // Call function on input args (or stdin if no args given)
    FILE* in = stdin;
    if (argind == argc || argc == 1) { 
        mygrep(keyword, in, out, case_sensitivity);
    } else {
        for (; argind < argc; argind++) {
            in = fopen(argv[argind], "r");
            if (in == NULL) {
                ERROR_EXIT(strerror(errno));
            }
            mygrep(keyword, in, out, case_sensitivity);
            if (fclose(in) != 0) {
                ERROR_EXIT(strerror(errno));
            }
        }
    }

    if (outfile != NULL) {
        if (fclose(out) != 0) {
            ERROR_EXIT(strerror(errno));
        }
    }

    exit(EXIT_SUCCESS);
}

/**
 * @brief Reduced variation of the Unix-command grep. Reads in several files and 
 * prints all lines containing a keyword.
 * 
 * @param keyword keyword to search for
 * @param in file to read from
 * @param out file to write output to
 * @param case_sensitivity defines whether search should be case-sensitive
 */
static void mygrep(char* keyword, FILE* in, FILE* out, bool case_sensitivity) {
    char* line = NULL;
    size_t len, buflen;

    while ((len = getline(&line, &buflen, in)) != -1) {

        // search string for occurences of keyword
        char* _line = strdup(line);
        if (_line == NULL) ERROR_EXIT(strerror(errno));
        char* _keyword = strdup(keyword);
        if (_keyword == NULL) ERROR_EXIT(strerror(errno));


        bool result = strcontains(_line, _keyword, case_sensitivity);
        
        if (result == true) {
            if (fputs(line, out) == EOF) {
                free(_keyword);
                free(_line);
                free(line);
                ERROR_EXIT("fputs failed");
            }
        }

        free(_keyword);
        free(_line);
        free(line);
        buflen = 0;
    } 

    free(line);
}

/**
 * @brief Checks if a line contains a keyword and returns the result
 * 
 * @param line string to search in
 * @param keyword keyword to search for
 * @param case_sensitivity whether search should be case-sensitive
 * @return true if line contains keyword
 * @return false if line does not contain keyword
 */
static bool strcontains(char* line, char* keyword, bool case_sensitivity) {
    if (case_sensitivity == false) {
        for (size_t i = 0; keyword[i]; i++) {
            keyword[i] = tolower(keyword[i]);
        }
        for (size_t i = 0; line[i]; i++) {
            line[i] = tolower(line[i]);
        }
    }

    return (strstr(line, keyword) != NULL);
}

static void ERROR_EXIT(char* message) {
    fprintf(stderr, "%s: %s\n", PROGRAM_NAME, message);
    exit(EXIT_FAILURE);
}

static void USAGE() {
    fprintf(stderr, "Usage: mygrep [-i] [-o outfile] keyword [file...]\n");
    exit(EXIT_FAILURE);
}