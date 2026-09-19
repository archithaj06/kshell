#!/usr/bin/env bash
# Run a command in kshell-vm (or open a shell if no args).
#   ./vm/ssh.sh                 -> interactive shell
#   ./vm/ssh.sh 'dmesg | tail'  -> run one command
exec ssh -i ~/.ssh/kshell_vm -p 2222 \
    -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR \
    kshell@127.0.0.1 "$@"
