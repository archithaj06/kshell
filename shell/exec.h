#ifndef KSHELL_EXEC_H
#define KSHELL_EXEC_H

#include "parser.h"

/*
 * Run a parsed pipeline and return the exit status of its last command
 * (0-255, 128+signal if killed by a signal, 127 if it could not be run).
 *
 * A lone builtin with no redirections runs in the shell process so that
 * `cd` and `exit` can affect it. Everything else is forked; builtins that
 * appear inside a pipeline or with redirections run in the child.
 */
int run_pipeline(const struct pipeline *pl);

#endif
