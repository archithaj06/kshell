#ifndef KSHELL_NOTE_H
#define KSHELL_NOTE_H

#define NOTE_DEVICE  "/dev/kshellnote"
#define NOTE_MAX_LEN 4096   /* matches NOTE_BUF_SIZE in the driver */

/* Replace the kernel note with `text` (a trailing newline is added). */
int note_write(const char *text);

/* Print the current kernel note to stdout. */
int note_read(void);

/* Discard the kernel note via ioctl. */
int note_clear(void);

/* Print the kernel note's length in bytes via ioctl. */
int note_len(void);

#endif
