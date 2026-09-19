/*
 * kshell - a minimal Unix shell.
 *
 * Read a line, parse it into a pipeline with redirections, run builtins
 * in-process and everything else via fork/exec. The `note` builtin talks to
 * the kshellnote character device driver in ../driver.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "exec.h"
#include "parser.h"

static void print_prompt(int last_status)
{
    if (last_status == 0)
        fputs("kshell> ", stdout);
    else
        printf("kshell[%d]> ", last_status);
    fflush(stdout);
}

int main(void)
{
    /* Ctrl-C should interrupt the running command, not the shell. Children
     * restore SIG_DFL before exec (see exec.c). */
    signal(SIGINT, SIG_IGN);

    char *line = NULL;
    size_t cap = 0;
    int last_status = 0;

    for (;;) {
        print_prompt(last_status);

        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) {
            if (errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            putchar('\n');          /* EOF (Ctrl-D): exit cleanly */
            break;
        }

        struct pipeline pl;
        int ncmds = parse_line(line, &pl);
        if (ncmds == 0)
            continue;
        if (ncmds < 0) {
            last_status = 2;
            continue;
        }

        last_status = run_pipeline(&pl);
    }

    free(line);
    return last_status;
}
