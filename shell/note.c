/*
 * Userspace side of the kshellnote driver. Nothing here is special: the
 * device is opened, read and written exactly like a regular file, and the
 * kernel routes those syscalls to the driver's file_operations. ioctl()
 * is the escape hatch for operations that are not reads or writes.
 */
#include "note.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int open_device(int slot, int flags)
{
    char path[32];
    snprintf(path, sizeof path, NOTE_DEVICE_FMT, slot);

    int fd = open(path, flags);
    if (fd < 0) {
        fprintf(stderr, "kshell: note: %s: %s\n", path, strerror(errno));
        if (errno == ENOENT)
            fprintf(stderr, "kshell: note: is the kshellnote module loaded?\n");
    }
    return fd;
}

int note_write(int slot, const char *text)
{
    int fd = open_device(slot, O_WRONLY);
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

int note_read(int slot)
{
    int fd = open_device(slot, O_RDONLY);
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

int note_clear(int slot)
{
    int fd = open_device(slot, O_WRONLY);
    if (fd < 0)
        return 1;

    int rc = 0;
    if (ioctl(fd, KSHELLNOTE_IOC_CLEAR) < 0) {
        fprintf(stderr, "kshell: note: ioctl(CLEAR): %s\n", strerror(errno));
        rc = 1;
    }
    close(fd);
    return rc;
}

int note_len(int slot)
{
    int fd = open_device(slot, O_RDONLY);
    if (fd < 0)
        return 1;

    int len = 0;
    int rc = 0;
    if (ioctl(fd, KSHELLNOTE_IOC_GETLEN, &len) < 0) {
        fprintf(stderr, "kshell: note: ioctl(GETLEN): %s\n", strerror(errno));
        rc = 1;
    } else {
        printf("%d\n", len);
    }
    close(fd);
    return rc;
}
