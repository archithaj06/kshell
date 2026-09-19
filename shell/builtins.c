#include "builtins.h"
#include "note.h"

#include <ctype.h>
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
 * note [N] write <text...>  -> replace note N with the joined words
 * note [N] read             -> print note N
 * note [N] clear            -> discard note N (ioctl)
 * note [N] len              -> print note N's length in bytes (ioctl)
 *
 * N selects /dev/kshellnoteN and defaults to 0.
 */
static int note_usage(void)
{
    fprintf(stderr, "usage: note [0-%d] write <text> | read | clear | len\n",
            KSHELLNOTE_COUNT - 1);
    return 2;
}

static int bi_note(char *const argv[])
{
    int slot = 0;
    int i = 1;

    /* Optional leading slot number. */
    if (argv[i] != NULL && isdigit((unsigned char)argv[i][0])) {
        char *end;
        long v = strtol(argv[i], &end, 10);
        if (*end != '\0' || v < 0 || v >= KSHELLNOTE_COUNT) {
            fprintf(stderr, "kshell: note: slot must be 0-%d\n", KSHELLNOTE_COUNT - 1);
            return 2;
        }
        slot = (int)v;
        i++;
    }

    const char *cmd = argv[i];
    if (cmd == NULL)
        return note_usage();
    char *const *rest = &argv[i + 1];

    if (rest[0] == NULL) {
        if (strcmp(cmd, "read") == 0)
            return note_read(slot);
        if (strcmp(cmd, "clear") == 0)
            return note_clear(slot);
        if (strcmp(cmd, "len") == 0)
            return note_len(slot);
        return note_usage();
    }

    if (strcmp(cmd, "write") != 0)
        return note_usage();

    /* Re-join the remaining words with single spaces. */
    char text[NOTE_MAX_LEN] = "";
    size_t used = 0;
    for (int j = 0; rest[j] != NULL; j++) {
        int n = snprintf(text + used, sizeof text - used, "%s%s",
                         j > 0 ? " " : "", rest[j]);
        if (n < 0 || (size_t)n >= sizeof text - used) {
            fprintf(stderr, "kshell: note: text too long (max %d bytes)\n", NOTE_MAX_LEN - 1);
            return 1;
        }
        used += (size_t)n;
    }
    return note_write(slot, text);
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
