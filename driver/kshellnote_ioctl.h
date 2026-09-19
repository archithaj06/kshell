/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ioctl interface of /dev/kshellnote, shared by the driver and userspace.
 *
 * <linux/ioctl.h> is a UAPI header, so it is valid in both worlds. The
 * magic byte should be unique per driver; the in-tree registry lives in
 * Documentation/userspace-api/ioctl/ioctl-number.rst.
 */
#ifndef KSHELLNOTE_IOCTL_H
#define KSHELLNOTE_IOCTL_H

#include <linux/ioctl.h>

#define KSHELLNOTE_IOC_MAGIC 'N'

/* Discard the note (note_len = 0). No argument. */
#define KSHELLNOTE_IOC_CLEAR  _IO(KSHELLNOTE_IOC_MAGIC, 1)

/* Store the current note length into the int pointed to by arg. */
#define KSHELLNOTE_IOC_GETLEN _IOR(KSHELLNOTE_IOC_MAGIC, 2, int)

#endif
