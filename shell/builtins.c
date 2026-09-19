#include "builtins.h"
#include "note.h"

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

/*
 * note write <text...>  -> replace the kernel note with the joined words
 * note read             -> print the kernel note
 */
static int bi_note(char *const argv[])
{
    if (argv[1] != NULL && strcmp(argv[1], "read") == 0 && argv[2] == NULL)
        return note_read();

    if (argv[1] == NULL || strcmp(argv[1], "write") != 0 || argv[2] == NULL) {
        fprintf(stderr, "usage: note write <text> | note read\n");
        return 2;
    }

    /* Re-join the remaining words with single spaces. */
    char text[NOTE_MAX_LEN] = "";
    size_t used = 0;
    for (int i = 2; argv[i] != NULL; i++) {
        int n = snprintf(text + used, sizeof text - used, "%s%s",
                         i > 2 ? " " : "", argv[i]);
        if (n < 0 || (size_t)n >= sizeof text - used) {
            fprintf(stderr, "kshell: note: text too long (max %d bytes)\n", NOTE_MAX_LEN - 1);
            return 1;
        }
        used += (size_t)n;
    }
    return note_write(text);
}

struct builtin {
    const char *name;
    int (*fn)(char *const argv[]);
};

static const struct builtin builtins[] = {
    { "cd",   bi_cd   },
    { "pwd",  bi_pwd  },
    { "exit", bi_exit },
    { "note", bi_note },
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
