#include "exec.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int run_external(char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("kshell: fork");
        return 127;
    }

    if (pid == 0) {
        /* Child: restore default Ctrl-C behaviour so the command can be
         * interrupted even though the shell itself ignores SIGINT. */
        signal(SIGINT, SIG_DFL);
        execvp(argv[0], argv);
        fprintf(stderr, "kshell: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("kshell: waitpid");
            return 127;
        }
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 0;
}
