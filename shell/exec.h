#ifndef KSHELL_EXEC_H
#define KSHELL_EXEC_H

/*
 * Fork, exec argv[0] via PATH lookup, and wait for it.
 * Returns the child's exit status (0-255), 128+signal if killed by a
 * signal, or 127 if the command could not be executed.
 */
int run_external(char *const argv[]);

#endif
