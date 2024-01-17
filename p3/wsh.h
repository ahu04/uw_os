#define _GNU_SOURCE
#include "parse.h"
#include "seq.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_JOBS 64

typedef struct {
    pid_t pid;
    bool running;
    char command[MAX_LINE];
} job_t;

typedef struct {
    job_t jobs[MAX_JOBS];
    int largestJobID;
} shell_ctx;

#define err_exit(msg)                                                          \
    do {                                                                       \
        fprintf(stderr, msg);                                                  \
        exit(-1);                                                              \
    } while (0)

void run_shell(FILE *file);
void batch_mode(char *filename);
void print_jobs(shell_ctx ctx[static 1]);
void foreground(char *id, shell_ctx ctx[static 1]);
void background(char *id, shell_ctx ctx[static 1]);

void execute_commands_foreground(char *currLine, shell_ctx ctx[static 1]);
void execute_commands_background(char *currLine, shell_ctx ctx[static 1]);

pid_t fork_program(Seq_T progs, char **prog_and_args, char *currLine, int inFD,
                   int outFD, int firstChildPid);
