#include "builtins.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int bi_cd(char *const argv[])
{
    const char *target = argv[1];
    if (target == NULL) {
        target = getenv("HOME");
        if (target == NULL) {
            fprintf(stderr, "kshell: cd: HOME not set\n");
            return 1;
        }
    }
    if (chdir(target) != 0) {
        fprintf(stderr, "kshell: cd: %s: %s\n", target, strerror(errno));
        return 1;
    }
    return 0;
}

static int bi_pwd(char *const argv[])
{
    (void)argv;
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof buf) == NULL) {
        perror("kshell: pwd");
        return 1;
    }
    puts(buf);
    return 0;
}

static int bi_exit(char *const argv[])
{
    int code = 0;
    if (argv[1] != NULL)
        code = atoi(argv[1]);
    exit(code & 0xff);
}

struct builtin {
    const char *name;
    int (*fn)(char *const argv[]);
};

static const struct builtin builtins[] = {
    { "cd",   bi_cd   },
    { "pwd",  bi_pwd  },
    { "exit", bi_exit },
};

int run_builtin(char *const argv[], int *status)
{
    for (size_t i = 0; i < sizeof builtins / sizeof builtins[0]; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0) {
            *status = builtins[i].fn(argv);
            return 1;
        }
    }
    return 0;
}
