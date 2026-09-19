# kshell

A small Unix shell written in C, plus a Linux character device driver it
talks to. The point of the project is to trace one request all the way
through the stack: a command typed at a prompt becomes a syscall, the kernel
dispatches it to a driver you wrote, and the result comes back out.

```
 kshell> note 1 write hello
         │
         │  write(fd, "hello\n", 6)            userspace  (shell/note.c)
 ────────┼────────────────────────────────────────────────────────────────
         ▼                                     kernel
   VFS: /dev/kshellnote1  (major 241, minor 1)
         │
         ▼
   note_write()  ──►  note_devs[1].buf  ◄──  note_read()
                      (4 KB, mutex)             ▲
   note_ioctl()  ──►  CLEAR / GETLEN            │  read(fd, buf, n)
                                                │
 kshell> note 1 read  ──────────────────────────┘
 hello
```

## Layout

```
shell/     userspace shell: REPL, parser, pipelines, builtins, note client
driver/    kshellnote kernel module (character devices), UAPI header, udev rule
vm/        VirtualBox provisioning template and ssh/sync helper scripts
```

### shell/

| File | Role |
|---|---|
| `main.c` | read–eval loop: prompt, `getline`, parse, run; ignores `SIGINT` so Ctrl-C only hits the child |
| `parser.c` | whitespace tokenizer → pipeline of commands with `<`, `>`, `>>` redirections |
| `exec.c` | one `fork` per stage, `pipe`/`dup2` wiring, redirections in the child, `execvp`, `waitpid` |
| `builtins.c` | `cd`, `pwd`, `exit`, `note` — a lone builtin runs in-process so it can change shell state |
| `note.c` | opens `/dev/kshellnoteN` and uses plain `open`/`read`/`write`/`ioctl` |

The prompt shows the last non-zero exit status: `kshell[127]>`. Operators
must be separated by whitespace (`ls | wc -l`, not `ls|wc -l`); quoting is
not supported.

### driver/

`kshellnote.c` registers `KSHELLNOTE_COUNT` (4) character devices under one
major:

1. `alloc_chrdev_region` reserves the major and 4 consecutive minors
2. per minor: `cdev_init` + `cdev_add` bind the `file_operations` table,
   `device_create` publishes it in sysfs so udev creates `/dev/kshellnoteN`
3. `99-kshellnote.rules` sets `MODE=0666` and symlinks `/dev/kshellnote` → `kshellnote0`

Each device is a `struct note_dev` with its own 4 KB buffer, length and
mutex. `open()` finds it with `container_of(inode->i_cdev, ...)` and stores
it in `filp->private_data` for later calls.

`read`/`write` use `simple_read_from_buffer` / `simple_write_to_buffer`. A
write starting at offset 0 replaces the note, `O_APPEND` extends it, and a
full buffer returns `-ENOSPC`. `kshellnote_ioctl.h` (shared with the shell)
defines `KSHELLNOTE_IOC_CLEAR` and `KSHELLNOTE_IOC_GETLEN`; unknown ioctls
return `-ENOTTY`. Teardown runs in the exact reverse order of init, and a
failure mid-init unwinds only what was created.

`class_create`'s signature changed in kernel 6.4; the source handles both.

## Building

Kernel work happens in a VM (see `vm/README.md`), never on the host.

```bash
# shell (any Linux, no root)
cd shell && make && ./kshell

# driver (inside the VM)
cd driver
make                 # builds kshellnote.ko against the running kernel
make install-udev    # once: node permissions + /dev/kshellnote symlink
make load            # sudo insmod
sudo dmesg | tail    # "kshellnote: loaded, major N, 4 notes (/dev/kshellnote0..3)"
make unload          # sudo rmmod
```

Sanity-check the driver without the shell:

```bash
echo "hi" > /dev/kshellnote2 && cat /dev/kshellnote2
echo "more" >> /dev/kshellnote2 && cat /dev/kshellnote2
```

## Try it

```
$ ./kshell
kshell> note write remember to take a VM snapshot
kshell> note 1 write groceries: milk eggs
kshell> note read
remember to take a VM snapshot
kshell> note 1 read | tr a-z A-Z
GROCERIES: MILK EGGS
kshell> note 1 len
21
kshell> note 1 clear
kshell> note read > backup.txt
kshell> cat < backup.txt | wc -w
6
kshell> ls /nonexistent
ls: cannot access '/nonexistent': No such file or directory
kshell[2]> exit
```

## Status

- [x] Phase 1 — basic shell
- [x] Phase 2 — hello-world module
- [x] Phase 3 — character device driver
- [x] Phase 4 — shell ↔ driver integration
- [x] Phase 5 — pipes/redirection, `ioctl` clear/len, multiple notes

Ideas for later: quoting and escapes in the parser, `poll()` support so a
reader can wait for a note to change, a `/proc` or sysfs view of all notes.
