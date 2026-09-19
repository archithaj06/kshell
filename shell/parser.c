#include "parser.h"

#include <string.h>

int parse_line(char *line, char *argv[MAX_ARGS])
{
    int argc = 0;
    char *save = NULL;
    char *tok = strtok_r(line, " \t\r\n", &save);

    while (tok != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = tok;
        tok = strtok_r(NULL, " \t\r\n", &save);
    }
    argv[argc] = NULL;
    return argc;
}
