#ifndef KSHELL_NOTE_H
#define KSHELL_NOTE_H

#include "kshellnote_ioctl.h"   /* KSHELLNOTE_COUNT, KSHELLNOTE_BUF_SIZE */

#define NOTE_DEVICE_FMT "/dev/kshellnote%d"
#define NOTE_MAX_LEN    KSHELLNOTE_BUF_SIZE

/* All functions act on note `slot` (0 .. KSHELLNOTE_COUNT-1). */

/* Replace the kernel note with `text` (a trailing newline is added). */
int note_write(int slot, const char *text);

/* Print the kernel note to stdout. */
int note_read(int slot);

/* Discard the kernel note via ioctl. */
int note_clear(int slot);

/* Print the kernel note's length in bytes via ioctl. */
int note_len(int slot);

#endif
