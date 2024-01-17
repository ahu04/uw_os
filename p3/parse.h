#include "seq.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_prog(Seq_T progs, char **old_prog_and_args, char *currLine);
size_t process_prog(char **prog_and_args[], char *progStr);
void populate_progs(Seq_T progs, char *currLine);
