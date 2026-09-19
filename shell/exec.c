#include "exec.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins.h"

static int status_from_wait(int wstatus)
{
    if (WIFEXITED(wstatus))
        return WEXITSTATUS(wstatus);
    if (WIFSIGNALED(wstatus))
        return 128 + WTERMSIG(wstatus);
    return 0;
}

/* Replace fd `to` with a freshly opened `path`. Returns -1 on failure. */
static int redirect(const char *path, int flags, int to)
{
    int fd = open(path, flags, 0644);
    if (fd < 0) {
        fprintf(stderr, "kshell: %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (dup2(fd, to) < 0) {
        perror("kshell: dup2");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

/* Runs in the child: wire up redirections, then exec or run the builtin. */
static void child_exec(const struct command *cmd)
{
    signal(SIGINT, SIG_DFL);

    if (cmd->in_file && redirect(cmd->in_file, O_RDONLY, STDIN_FILENO) < 0)
        _exit(1);
    if (cmd->out_file) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        if (redirect(cmd->out_file, flags, STDOUT_FILENO) < 0)
            _exit(1);
    }

    int status;
    if (run_builtin(cmd->argv, &status)) {
        fflush(stdout);
        _exit(status);
    }

    execvp(cmd->argv[0], cmd->argv);
    fprintf(stderr, "kshell: %s: %s\n", cmd->argv[0], strerror(errno));
    _exit(127);
}

int run_pipeline(const struct pipeline *pl)
{
    const struct command *first = &pl->cmds[0];
    int status = 0;

    if (pl->ncmds == 1 && !first->in_file && !first->out_file &&
        run_builtin(first->argv, &status))
        return status;

    pid_t pids[MAX_CMDS];
    int prev_read = -1;          /* read end of the previous stage's pipe */

    for (int i = 0; i < pl->ncmds; i++) {
        int pfd[2] = { -1, -1 };
        int last = (i == pl->ncmds - 1);

        if (!last && pipe(pfd) < 0) {
            perror("kshell: pipe");
            return 127;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("kshell: fork");
            return 127;
        }

        if (pids[i] == 0) {
            if (prev_read >= 0) {
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (!last) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[0]);
                close(pfd[1]);
            }
            child_exec(&pl->cmds[i]);
        }

        /* Parent: keep only what the next stage needs. */
        if (prev_read >= 0)
            close(prev_read);
        if (!last) {
            close(pfd[1]);
            prev_read = pfd[0];
        }
    }

    for (int i = 0; i < pl->ncmds; i++) {
        int wstatus = 0;
        while (waitpid(pids[i], &wstatus, 0) < 0) {
            if (errno != EINTR) {
                perror("kshell: waitpid");
                return 127;
            }
        }
        status = status_from_wait(wstatus);   /* last one wins */
    }
    return status;
}
