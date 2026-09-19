#!/usr/bin/env bash
# Copy the working tree (minus .git and build artifacts) into ~/kshell on the VM.
set -euo pipefail
cd "$(dirname "$0")/.."
tar --exclude=.git --exclude='*.o' --exclude='*.ko' --exclude='shell/kshell' -cf - . \
  | ./vm/ssh.sh 'rm -rf ~/kshell && mkdir -p ~/kshell && tar -xf - -C ~/kshell'
echo "synced to kshell-vm:~/kshell"
