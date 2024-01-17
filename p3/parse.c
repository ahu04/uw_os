#include "parse.h"

void free_prog(Seq_T progs, char **old_prog_and_args, char *currLine) {
    // sometimes I wish I had garbage collection yanno

    int len = Seq_length(progs);

    for (int i = 0; i < len; ++i) {
        char **curr_prog_and_args = (char **)Seq_get(progs, i);
        free(curr_prog_and_args);
    }
    Seq_free(&progs);
    free(old_prog_and_args);
    free(currLine);
}

// processes a command line input stored in currLine, separating parameters
// and storing all parameters in *prog_and_args. The corrollary is
// *prog_and_args[i] = char *argv[i], more or less caller responsible for
// freeing this memory.

// returns number of args (think argc).
size_t process_prog(char **prog_and_args[], char *progStr) {
    char *token;
    size_t i = 0;
    size_t max_sz = 10;
    char **args = (char **)malloc(max_sz * sizeof(char *));
    if (args == NULL) {
        fprintf(stderr, "malloc fail\n");
        exit(EXIT_FAILURE);
    }

    while ((token = strtok_r(progStr, " &", &progStr)) != NULL) {
        if (i == max_sz - 1) {
            args = (char **)realloc(args, max_sz * 2);
            if (args == NULL) {
                fprintf(stderr, "malloc fail\n");
                exit(EXIT_FAILURE);
            }
            max_sz *= 2;
        }
        args[i] = token;
        ++i;
    }
    args[i] = NULL;
    *prog_and_args = args;
    return i;
}

void populate_progs(Seq_T progs, char *currLine) {
    char *token;
    char **prog_and_args;

    while ((token = strtok_r(currLine, "|", &currLine)) != NULL) {
        process_prog(&prog_and_args, token);
        Seq_addhi(progs, (void *)prog_and_args);
    }
}