#ifndef KSHELL_PARSER_H
#define KSHELL_PARSER_H

#define MAX_ARGS 64

/*
 * Split `line` in place on whitespace into a NULL-terminated argv.
 * Returns the number of tokens (0 for an empty/blank line).
 * The strings in argv point into `line`, so `line` must outlive argv.
 */
int parse_line(char *line, char *argv[MAX_ARGS]);

#endif
