// SPDX-License-Identifier: GPL-2.0
/*
 * kshellnote - a character device holding one small text note.
 *
 * Userspace writes a note into /dev/kshellnote and reads it back. The note
 * lives in a fixed kernel buffer guarded by a mutex. A write that starts at
 * offset 0 replaces the note; O_APPEND writes extend it.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "kshellnote_ioctl.h"

#define DEVICE_NAME   "kshellnote"
#define CLASS_NAME    "kshell"
#define NOTE_BUF_SIZE 4096

static char note_buf[NOTE_BUF_SIZE];
static size_t note_len;                 /* bytes of note_buf in use */
static DEFINE_MUTEX(note_lock);

static dev_t note_devno;
static struct cdev note_cdev;
static struct class *note_class;

static int note_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int note_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t note_read(struct file *filp, char __user *ubuf, size_t count,
			 loff_t *ppos)
{
	ssize_t ret;

	if (mutex_lock_interruptible(&note_lock))
		return -ERESTARTSYS;
	/* Copies min(count, note_len - *ppos) bytes and advances *ppos.
	 * Returns 0 once *ppos reaches note_len, which is how `cat` sees EOF. */
	ret = simple_read_from_buffer(ubuf, count, ppos, note_buf, note_len);
	mutex_unlock(&note_lock);
	return ret;
}

static ssize_t note_write(struct file *filp, const char __user *ubuf,
			  size_t count, loff_t *ppos)
{
	ssize_t ret;

	if (mutex_lock_interruptible(&note_lock))
		return -ERESTARTSYS;

	if (filp->f_flags & O_APPEND)
		*ppos = note_len;       /* the VFS does not do this for char devs */
	else if (*ppos == 0)
		note_len = 0;           /* a fresh write replaces the note */

	ret = simple_write_to_buffer(note_buf, NOTE_BUF_SIZE, ppos, ubuf, count);
	if (ret > 0 && (size_t)*ppos > note_len)
		note_len = *ppos;
	else if (ret == 0 && count > 0)
		ret = -ENOSPC;          /* buffer full: tell userspace instead of looping */

	mutex_unlock(&note_lock);
	return ret;
}

/*
 * ioctl: control operations that do not fit read/write. `cmd` encodes
 * direction, size, magic and number (see kshellnote_ioctl.h); `arg` is a
 * user pointer or plain integer depending on the command.
 */
static long note_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int len;

	switch (cmd) {
	case KSHELLNOTE_IOC_CLEAR:
		if (mutex_lock_interruptible(&note_lock))
			return -ERESTARTSYS;
		note_len = 0;
		mutex_unlock(&note_lock);
		return 0;

	case KSHELLNOTE_IOC_GETLEN:
		if (mutex_lock_interruptible(&note_lock))
			return -ERESTARTSYS;
		len = note_len;
		mutex_unlock(&note_lock);
		/* put_user checks the pointer and copies one int to userspace. */
		return put_user(len, (int __user *)arg);

	default:
		return -ENOTTY;         /* "not a typewriter": unknown ioctl */
	}
}

static const struct file_operations note_fops = {
	.owner          = THIS_MODULE,
	.open           = note_open,
	.release        = note_release,
	.read           = note_read,
	.write          = note_write,
	.unlocked_ioctl = note_ioctl,
};

static int __init kshellnote_init(void)
{
	int ret;
	struct device *dev;

	/* 1. Reserve a (major, minor) pair; the major shows up in /proc/devices. */
	ret = alloc_chrdev_region(&note_devno, 0, 1, DEVICE_NAME);
	if (ret) {
		pr_err("kshellnote: alloc_chrdev_region failed: %d\n", ret);
		return ret;
	}

	/* 2. Bind our file_operations to that device number. */
	cdev_init(&note_cdev, &note_fops);
	note_cdev.owner = THIS_MODULE;
	ret = cdev_add(&note_cdev, note_devno, 1);
	if (ret) {
		pr_err("kshellnote: cdev_add failed: %d\n", ret);
		goto err_unregister;
	}

	/* 3. Create a sysfs class + device so udev creates /dev/kshellnote. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	note_class = class_create(CLASS_NAME);
#else
	note_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(note_class)) {
		ret = PTR_ERR(note_class);
		pr_err("kshellnote: class_create failed: %d\n", ret);
		goto err_cdev;
	}

	dev = device_create(note_class, NULL, note_devno, NULL, DEVICE_NAME);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("kshellnote: device_create failed: %d\n", ret);
		goto err_class;
	}

	pr_info("kshellnote: loaded, major %d, /dev/%s\n", MAJOR(note_devno),
		DEVICE_NAME);
	return 0;

err_class:
	class_destroy(note_class);
err_cdev:
	cdev_del(&note_cdev);
err_unregister:
	unregister_chrdev_region(note_devno, 1);
	return ret;
}

static void __exit kshellnote_exit(void)
{
	/* Tear down in exactly the reverse order of init. */
	device_destroy(note_class, note_devno);
	class_destroy(note_class);
	cdev_del(&note_cdev);
	unregister_chrdev_region(note_devno, 1);
	pr_info("kshellnote: unloaded\n");
}

module_init(kshellnote_init);
module_exit(kshellnote_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hemanth Simhadri");
MODULE_DESCRIPTION("kshell note buffer character device");
