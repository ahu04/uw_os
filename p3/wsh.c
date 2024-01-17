#include "wsh.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        run_shell(stdin);
    } else if (argc == 2) {
        batch_mode(argv[1]);
    } else {
        err_exit("usage: ./wsh [script]\n");
    }
    return 0;
}

void run_shell(FILE *file) {
    int eofMarker = 0;
    shell_ctx ctx = {0};
    while (eofMarker != EOF) {
        if (file == stdin)
            printf("wsh$ ");

        char *currLine = NULL;
        size_t len = 0;
        eofMarker = getline(&currLine, &len, file);
        // trim trailing \n, check for trailing &, then parse
        int last = strlen(currLine);
        currLine[last - 1] = '\0';
        bool runInBackground = (last > 1) ? (currLine[last - 2] == '&') : false;

        if (strcmp(currLine, "\0") == 0) {
            free(currLine);
            continue;
        } else if (strcmp(currLine, "exit") == 0) {
            free(currLine);
            exit(EXIT_SUCCESS);
        } else if (strncmp(currLine, "cd ", 3) == 0) {
            int ret = chdir((char *)currLine + 3);
            if (ret != 0) {
                free(currLine);
                err_exit("chdir failed\n");
            }
        } else if (strcmp(currLine, "jobs") == 0) {
            print_jobs(&ctx);
        } else if (strncmp(currLine, "fg", 2) == 0) {
            foreground((char *)currLine + 2, &ctx);
        } else if (strncmp(currLine, "bg", 2) == 0) {
            background((char *)currLine + 2, &ctx);
        } else {
            runInBackground ? execute_commands_background(currLine, &ctx)
                            : execute_commands_foreground(currLine, &ctx);
        }
        free(currLine);
    }
}

void batch_mode(char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        err_exit("failed to open script file in batch mode\n");
    }
    FILE *file = fdopen(fd, "r");
    if (file == NULL) {
        err_exit("failed to convert fd to FILE*\n");
    }
    run_shell(file);
}

void print_jobs(shell_ctx ctx[static 1]) {
    int status;
    for (int i = 0; i <= ctx->largestJobID; i++) {
        // if process has exited
        if (ctx->jobs[i].running &&
            waitpid(ctx->jobs[i].pid, &status, WNOHANG) == ctx->jobs[i].pid &&
            WIFEXITED(status)) {
            ctx->jobs[i].running = false;
        } else if (ctx->jobs[i].running) {
            // process still running in bg
            printf("%d: %s\n", i + 1, ctx->jobs[i].command);
        }
    }
}

void foreground(char *id, shell_ctx ctx[static 1]) {
    pid_t shell_pgid = tcgetpgrp(STDIN_FILENO);

    int job_id;
    char *jobIdStr = strchr(id, '%');
    if (jobIdStr == NULL) {
        // no arg properly passed to "fg"
        job_id = ctx->largestJobID;
        // if the prev largest job ID is now inactive, find most recent active
        // (if exists) and update ctx largest job
        if (!ctx->jobs[job_id].running) {
            for (int i = job_id; i >= 0; i--) {
                if (ctx->jobs[i].running) {
                    job_id = i;
                    ctx->largestJobID = i;
                    break;
                }
            }
        }
    } else {
        job_id = atoi((char *)jobIdStr + 1) - 1;
    }
    job_t job = ctx->jobs[job_id];
    if (!job.running) {
        printf("wsh: fg: %s: no such job\n",
               (jobIdStr == NULL) ? "current" : jobIdStr);
        return;
    }
    pid_t pgid = getpgid(job.pid);
    // continue job if paused
    int ret = kill(-pgid, SIGCONT);
    if (ret == -1) {
        err_exit("SIGCONT failure\n");
    }
    // ignore SIGTTOU in parent (aka shell)
    signal(SIGTTOU, SIG_IGN);
    ret = tcsetpgrp(STDIN_FILENO, pgid);
    if (ret == -1) {
        err_exit("set process group failure\n");
    }
    int status;
    waitpid(job.pid, &status, WUNTRACED);
    // update context if job terminated, wait on remaining processes in job
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        ctx->jobs[job_id].running = false;
        while (waitpid(-pgid, &status, 0) > 0) {

        }
    }
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void background(char *id, shell_ctx ctx[const static 1]) {
    int job_id;
    char *jobIdStr = strchr(id, '%');
    if (jobIdStr == NULL) {
        // no arg properly passed to "fg"
        job_id = ctx->largestJobID;
        // if the prev largest job ID is now inactive, find most recent active
        // (if exists) and update ctx largest job
        if (!ctx->jobs[job_id].running) {
            for (int i = job_id; i >= 0; i--) {
                if (ctx->jobs[i].running) {
                    job_id = i;
                    ctx->largestJobID = i;
                    break;
                }
            }
        }
    } else {
        job_id = atoi((char *)jobIdStr + 1) - 1;
    }
    // send sigcont to job (that is already in background, potentially stopped)
    job_t job = ctx->jobs[job_id];
    if (!job.running) {
        printf("wsh: bg: %s: no such job\n",
               (jobIdStr == NULL) ? "current" : jobIdStr);
        return;
    }
    pid_t pgid = getpgid(job.pid);
    // continue job if paused
    int ret = kill(-pgid, SIGCONT);
    if (ret == -1) {
        err_exit("SIGCONT failure\n");
    }
    return;
}

void execute_commands_foreground(char *currLine, shell_ctx ctx[static 1]) {

    char original[MAX_LINE] = {0};
    strcpy(original, currLine);

    Seq_T progs = Seq_new(5);

    if (currLine == NULL) {
        err_exit("bad things have happened\n");
    }

    populate_progs(progs, currLine);

    int len = Seq_length(progs);
    int pipefd[2];
    int inFD = STDIN_FILENO;
    int firstChildPid = 0;

    for (int i = 0; i < len - 1; ++i) {
        if (pipe(pipefd) == -1) {
            err_exit("pipe failed\n");
        }
        char **prog_and_args = (char **)Seq_remlo(progs);
        if (i == 0) {
            firstChildPid = fork_program(progs, prog_and_args, currLine, inFD,
                                         pipefd[1], 0);
        } else {
            fork_program(progs, prog_and_args, currLine, inFD, pipefd[1],
                         firstChildPid);
        }
        // close new write pipe, child writes not parent
        close(pipefd[1]);
        // close old read pipe
        if (inFD != STDIN_FILENO) {
            close(inFD);
        }
        // set output of current pipe to input pipe for next iteration
        inFD = pipefd[0];
    }

    if (len > 0) {
        char **prog_and_args = (char **)Seq_remlo(progs);
        pid_t last_child = fork_program(progs, prog_and_args, currLine, inFD,
                                      STDOUT_FILENO, firstChildPid);

        // ignore SIGTTOU in parent (aka shell)
        signal(SIGTTOU, SIG_IGN);
        signal(SIGSTOP, SIG_DFL);
        pid_t shell_pgid = tcgetpgrp(STDIN_FILENO);

        // if run in foreground, move job to foreground, wait for job to finish.
        int ret = tcsetpgrp(STDIN_FILENO, getpgid(last_child));
        if (ret == -1) {
            err_exit("set process group failure\n");
        }

        int childstatus;
        waitpid(last_child, &childstatus, WUNTRACED);

        for (int i = 0; i < len - 1; i++) {
            wait(NULL);
        }

        if (WIFSTOPPED(childstatus)) {
            // update shell context w/ paused job
            for (int i = 0; i < MAX_JOBS; i++) {
                // find lowest unused background ID. This is 0-indexed, on terminal
                // display is 1-indexed
                if (!ctx->jobs[i].running) {
                    ctx->jobs[i].running = true;
                    ctx->jobs[i].pid = last_child;
                    // if smaller ID was not recycled
                    if (i > ctx->largestJobID) {
                        ctx->largestJobID = i;
                    }
                    memcpy(ctx->jobs[i].command, original, MAX_LINE);
                    break;
                }
            }
        }
        // give shell back control after job finishes / stopped
        tcsetpgrp(STDIN_FILENO, shell_pgid);
    }
    Seq_free(&progs);
}

void execute_commands_background(char *currLine, shell_ctx ctx[static 1]) {
    char original[MAX_LINE] = {0};
    strcpy(original, currLine);

    Seq_T progs = Seq_new(5);

    if (currLine == NULL) {
        err_exit("bad things have happened\n");
    }

    populate_progs(progs, currLine);

    int len = Seq_length(progs);
    int pipefd[2];
    int inFD = STDIN_FILENO;
    pid_t last_child = 0;
    int firstChildPid;

    for (int i = 0; i < len; ++i) {
        if (i < len - 1) {
            if (pipe(pipefd) == -1) {
                err_exit("pipe failed\n");
            }
        }
        char **prog_and_args = (char **)Seq_remlo(progs);
        if (i == 0) {
            firstChildPid = last_child =
                fork_program(progs, prog_and_args, currLine, inFD,
                             (i == len - 1) ? STDOUT_FILENO : pipefd[1], 0);
        } else {
            last_child = fork_program(
                progs, prog_and_args, currLine, inFD,
                (i == len - 1) ? STDOUT_FILENO : pipefd[1], firstChildPid);
        }
        // close new write pipe, child writes not parent
        if (i < len - 1) {
            close(pipefd[1]);
        }
        // close old read pipe
        if (inFD != STDIN_FILENO) {
            close(inFD);
        }
        // set output of current pipe to input pipe for next iteration
        inFD = pipefd[0];
    }
    // update shell context
    for (int i = 0; i < MAX_JOBS; i++) {
        // find lowest unused background ID. This is 0-indexed, on terminal
        // display is 1-indexed
        if (!ctx->jobs[i].running) {
            ctx->jobs[i].running = true;
            ctx->jobs[i].pid = last_child;
            // if smaller ID was not recycled
            if (i > ctx->largestJobID) {
                ctx->largestJobID = i;
            }
            memcpy(ctx->jobs[i].command, original, MAX_LINE);
            break;
        }
    }
    Seq_free(&progs);
}

// fork_program is called with an in and out file descriptor, which
// is used to pipe IO between commands.

// fork_program responsible for freeing char **prog_and_args, and upon failure,
// frees all associated heap memory w/ current process.
pid_t fork_program(Seq_T progs, char **prog_and_args, char *currLine, int inFD,
                   int outFD, int firstChildPid) {

    if (strcmp(prog_and_args[0], "exit") == 0) {
        free_prog(progs, prog_and_args, currLine);
        exit(EXIT_SUCCESS);
    }

    pid_t pid = fork();

    if (pid < 0) {
        free_prog(progs, prog_and_args, currLine);
        err_exit("fork failed\n");
    } else if (pid == 0) {
        // if not standard in/standard out (i.e. piping to some
        // other file descriptor, then
        // we set STDIN/OUT to our custom file descriptors.
        if (inFD != STDIN_FILENO) {
            if (dup2(inFD, STDIN_FILENO) != STDIN_FILENO) {
                err_exit("dup2 failed\n");
            }
            close(inFD);
        }
        if (outFD != STDOUT_FILENO) {
            if (dup2(outFD, STDOUT_FILENO) != STDOUT_FILENO) {
                err_exit("dup2 failed\n");
            }
            close(outFD);
        }
        // this is child, run the shell cmd given by user.
        execvp(prog_and_args[0], prog_and_args);

        // only triggers when execvp fails.
        fprintf(stderr, "%s\n", strerror(errno));
        fprintf(stderr, "wsh error: Command not found: %s\n", prog_and_args[0]);
        free_prog(progs, prog_and_args, currLine);
        exit(127);
    } else {
        // parent process, set process group of children to separate group
        setpgid(pid, firstChildPid);
        free(prog_and_args);
        return pid;
    }
}