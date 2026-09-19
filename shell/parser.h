#ifndef KSHELL_PARSER_H
#define KSHELL_PARSER_H

#define MAX_ARGS 64
#define MAX_CMDS 8

/* One simple command: argv plus optional redirections. */
struct command {
    char *argv[MAX_ARGS];
    const char *in_file;    /* "< file"            */
    const char *out_file;   /* "> file" / ">> file" */
    int append;             /* 1 for ">>"          */
};

/* A pipeline: cmd0 | cmd1 | ... | cmdN-1 */
struct pipeline {
    struct command cmds[MAX_CMDS];
    int ncmds;
};

/*
 * Tokenize `line` in place on whitespace and split it into a pipeline.
 * Operators (|, <, >, >>) must be separated from their neighbours by
 * whitespace. Returns the number of commands, 0 for a blank line, or -1
 * after printing a syntax error. Strings in the result point into `line`.
 */
int parse_line(char *line, struct pipeline *pl);

#endif
