/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Userspace-facing interface of the kshellnote devices, shared by the
 * driver and the shell.
 *
 * <linux/ioctl.h> is a UAPI header, so it is valid in both worlds. The
 * ioctl magic byte should be unique per driver; the in-tree registry lives
 * in Documentation/userspace-api/ioctl/ioctl-number.rst.
 */
#ifndef KSHELLNOTE_IOCTL_H
#define KSHELLNOTE_IOCTL_H

#include <linux/ioctl.h>

/* Number of note devices: /dev/kshellnote0 .. /dev/kshellnote(COUNT-1). */
#define KSHELLNOTE_COUNT 4

/* Capacity of each note in bytes. */
#define KSHELLNOTE_BUF_SIZE 4096

#define KSHELLNOTE_IOC_MAGIC 'N'

/* Discard the note (length becomes 0). No argument. */
#define KSHELLNOTE_IOC_CLEAR  _IO(KSHELLNOTE_IOC_MAGIC, 1)

/* Store the current note length into the int pointed to by arg. */
#define KSHELLNOTE_IOC_GETLEN _IOR(KSHELLNOTE_IOC_MAGIC, 2, int)

#endif
