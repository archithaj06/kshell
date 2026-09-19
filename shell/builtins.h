#ifndef KSHELL_BUILTINS_H
#define KSHELL_BUILTINS_H

/*
 * If argv[0] names a builtin, run it in the current process and return 1,
 * storing its exit status in *status. Otherwise return 0 and leave *status
 * untouched. Builtins must run in-process because they mutate shell state
 * (cwd, exit).
 */
int run_builtin(char *const argv[], int *status);

#endif
