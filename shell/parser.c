#include "parser.h"

#include <stdio.h>
#include <string.h>

static int syntax_error(const char *msg)
{
    fprintf(stderr, "kshell: syntax error: %s\n", msg);
    return -1;
}

int parse_line(char *line, struct pipeline *pl)
{
    memset(pl, 0, sizeof *pl);

    struct command *cmd = &pl->cmds[0];
    int argc = 0;
    char *save = NULL;
    char *tok = strtok_r(line, " \t\r\n", &save);

    if (tok == NULL)
        return 0;

    for (; tok != NULL; tok = strtok_r(NULL, " \t\r\n", &save)) {
        if (strcmp(tok, "|") == 0) {
            if (argc == 0)
                return syntax_error("empty command before '|'");
            if (pl->ncmds + 1 >= MAX_CMDS)
                return syntax_error("too many commands in pipeline");
            cmd->argv[argc] = NULL;
            cmd = &pl->cmds[++pl->ncmds];
            argc = 0;
            continue;
        }

        if (strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0 || strcmp(tok, ">>") == 0) {
            const char *op = tok;
            char *file = strtok_r(NULL, " \t\r\n", &save);
            if (file == NULL)
                return syntax_error("missing file name after redirection");
            if (op[0] == '<') {
                cmd->in_file = file;
            } else {
                cmd->out_file = file;
                cmd->append = (op[1] == '>');
            }
            continue;
        }

        if (argc >= MAX_ARGS - 1)
            return syntax_error("too many arguments");
        cmd->argv[argc++] = tok;
    }

    if (argc == 0)
        return syntax_error("empty command after '|'");
    cmd->argv[argc] = NULL;
    return ++pl->ncmds;
}
