# kshell

A small Unix shell written in C, plus a Linux character device driver it
talks to. The point of the project is to trace one request all the way
through the stack: a command typed at a prompt becomes a syscall, the kernel
dispatches it to a driver you wrote, and the result comes back out.

```
 kshell> note write hello
         │
         │  write(fd, "hello\n", 6)          userspace  (shell/note.c)
 ────────┼──────────────────────────────────────────────────────────────
         ▼                                   kernel
   VFS: /dev/kshellnote  (major 241)
         │
         ▼
   note_write()  ──►  note_buf[4096]  ◄──  note_read()
                      (mutex-guarded)         ▲
                                              │  read(fd, buf, n)
 kshell> note read  ──────────────────────────┘
 hello
```

## Layout

```
shell/     userspace shell: REPL, tokenizer, fork/exec, builtins, note client
driver/    kshellnote kernel module (character device) + udev rule
vm/        VirtualBox provisioning template and ssh/sync helper scripts
```

### shell/

| File | Role |
|---|---|
| `main.c` | read–eval loop: prompt, `getline`, dispatch; ignores `SIGINT` so Ctrl-C only hits the child |
| `parser.c` | whitespace tokenizer → `argv[]` |
| `exec.c` | `fork` / `execvp` / `waitpid`; child restores default `SIGINT`; returns exit status |
| `builtins.c` | `cd`, `pwd`, `exit`, `note` — run in-process because they change shell state |
| `note.c` | opens `/dev/kshellnote` and uses plain `open`/`read`/`write` |

The prompt shows the last non-zero exit status: `kshell[127]>`.

### driver/

`kshellnote.c` registers one character device:

1. `alloc_chrdev_region` reserves a major/minor (visible in `/proc/devices`)
2. `cdev_init` + `cdev_add` bind the `file_operations` table to it
3. `class_create` + `device_create` publish it in sysfs so udev creates
   `/dev/kshellnote`; `99-kshellnote.rules` sets `MODE=0666`

`read` and `write` use `simple_read_from_buffer` / `simple_write_to_buffer`
over a 4 KB static buffer guarded by a mutex. A write starting at offset 0
replaces the note, `O_APPEND` extends it, and a full buffer returns
`-ENOSPC`. Teardown runs in the exact reverse order of init.

`class_create`'s signature changed in kernel 6.4; the source handles both.

## Building

Kernel work happens in a VM (see `vm/README.md`), never on the host.

```bash
# shell (any Linux, no root)
cd shell && make && ./kshell

# driver (inside the VM)
cd driver
make                 # builds kshellnote.ko against the running kernel
make install-udev    # once: lets non-root users open /dev/kshellnote
make load            # sudo insmod
sudo dmesg | tail    # "kshellnote: loaded, major N, /dev/kshellnote"
make unload          # sudo rmmod
```

Sanity-check the driver without the shell:

```bash
echo "hi" > /dev/kshellnote && cat /dev/kshellnote
echo "more" >> /dev/kshellnote && cat /dev/kshellnote
```

## Try it

```
$ ./kshell
kshell> note write remember to take a VM snapshot
kshell> note read
remember to take a VM snapshot
kshell> ls /nonexistent
ls: cannot access '/nonexistent': No such file or directory
kshell[2]> exit
```

## Status

- [x] Phase 1 — basic shell
- [x] Phase 2 — hello-world module
- [x] Phase 3 — character device driver
- [x] Phase 4 — shell ↔ driver integration
- [ ] Phase 5 — pipes/redirection, `ioctl` to clear, named notes
