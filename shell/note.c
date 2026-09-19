/*
 * Userspace side of the kshellnote driver. Nothing here is special: the
 * device is opened, read and written exactly like a regular file, and the
 * kernel routes those syscalls to the driver's file_operations.
 */
#include "note.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int open_device(int flags)
{
    int fd = open(NOTE_DEVICE, flags);
    if (fd < 0) {
        fprintf(stderr, "kshell: note: %s: %s\n", NOTE_DEVICE, strerror(errno));
        if (errno == ENOENT)
            fprintf(stderr, "kshell: note: is the kshellnote module loaded?\n");
    }
    return fd;
}

int note_write(const char *text)
{
    int fd = open_device(O_WRONLY);
    if (fd < 0)
        return 1;

    size_t len = strlen(text);
    ssize_t n = write(fd, text, len);
    if (n >= 0 && (size_t)n == len)
        n = write(fd, "\n", 1);

    int rc = 0;
    if (n < 0) {
        fprintf(stderr, "kshell: note: write: %s\n", strerror(errno));
        rc = 1;
    }
    close(fd);
    return rc;
}

int note_read(void)
{
    int fd = open_device(O_RDONLY);
    if (fd < 0)
        return 1;

    char buf[512];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0)
        fwrite(buf, 1, (size_t)n, stdout);

    int rc = 0;
    if (n < 0) {
        fprintf(stderr, "kshell: note: read: %s\n", strerror(errno));
        rc = 1;
    }
    close(fd);
    return rc;
}
