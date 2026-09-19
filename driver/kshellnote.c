// SPDX-License-Identifier: GPL-2.0
/*
 * kshellnote - character devices holding small text notes.
 *
 * The module registers KSHELLNOTE_COUNT minors under one major. Each minor
 * (/dev/kshellnote0, /dev/kshellnote1, ...) is an independent note with
 * its own buffer and mutex. Userspace writes a note in and reads it back;
 * a write that starts at offset 0 replaces the note, O_APPEND writes
 * extend it, and two ioctls clear it or report its length.
 *
 * Per-device state is found from the cdev embedded in each struct note_dev
 * (container_of in open) and cached in filp->private_data for read/write.
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

#define DEVICE_NAME "kshellnote"
#define CLASS_NAME  "kshell"

struct note_dev {
	char buf[KSHELLNOTE_BUF_SIZE];
	size_t len;                     /* bytes of buf in use */
	struct mutex lock;
	struct cdev cdev;
};

static struct note_dev note_devs[KSHELLNOTE_COUNT];
static dev_t note_base;                 /* major + first minor */
static struct class *note_class;

static int note_open(struct inode *inode, struct file *filp)
{
	/* inode->i_cdev is the cdev we registered for this minor; walk back
	 * to the enclosing note_dev and remember it for later calls. */
	filp->private_data = container_of(inode->i_cdev, struct note_dev, cdev);
	return 0;
}

static int note_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t note_read(struct file *filp, char __user *ubuf, size_t count,
			 loff_t *ppos)
{
	struct note_dev *d = filp->private_data;
	ssize_t ret;

	if (mutex_lock_interruptible(&d->lock))
		return -ERESTARTSYS;
	/* Copies min(count, len - *ppos) bytes and advances *ppos. Returns 0
	 * once *ppos reaches len, which is how `cat` sees EOF. */
	ret = simple_read_from_buffer(ubuf, count, ppos, d->buf, d->len);
	mutex_unlock(&d->lock);
	return ret;
}

static ssize_t note_write(struct file *filp, const char __user *ubuf,
			  size_t count, loff_t *ppos)
{
	struct note_dev *d = filp->private_data;
	ssize_t ret;

	if (mutex_lock_interruptible(&d->lock))
		return -ERESTARTSYS;

	if (filp->f_flags & O_APPEND)
		*ppos = d->len;         /* the VFS does not do this for char devs */
	else if (*ppos == 0)
		d->len = 0;             /* a fresh write replaces the note */

	ret = simple_write_to_buffer(d->buf, KSHELLNOTE_BUF_SIZE, ppos, ubuf, count);
	if (ret > 0 && (size_t)*ppos > d->len)
		d->len = *ppos;
	else if (ret == 0 && count > 0)
		ret = -ENOSPC;          /* buffer full: tell userspace instead of looping */

	mutex_unlock(&d->lock);
	return ret;
}

/*
 * ioctl: control operations that do not fit read/write. `cmd` encodes
 * direction, size, magic and number (see kshellnote_ioctl.h); `arg` is a
 * user pointer or plain integer depending on the command.
 */
static long note_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct note_dev *d = filp->private_data;
	int len;

	switch (cmd) {
	case KSHELLNOTE_IOC_CLEAR:
		if (mutex_lock_interruptible(&d->lock))
			return -ERESTARTSYS;
		d->len = 0;
		mutex_unlock(&d->lock);
		return 0;

	case KSHELLNOTE_IOC_GETLEN:
		if (mutex_lock_interruptible(&d->lock))
			return -ERESTARTSYS;
		len = d->len;
		mutex_unlock(&d->lock);
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

/* Undo device_create/cdev_add for minors [0, n). */
static void note_teardown(int n)
{
	while (n-- > 0) {
		device_destroy(note_class, MKDEV(MAJOR(note_base), n));
		cdev_del(&note_devs[n].cdev);
	}
}

static int __init kshellnote_init(void)
{
	int ret, i;

	/* 1. Reserve a major with KSHELLNOTE_COUNT consecutive minors. */
	ret = alloc_chrdev_region(&note_base, 0, KSHELLNOTE_COUNT, DEVICE_NAME);
	if (ret) {
		pr_err("kshellnote: alloc_chrdev_region failed: %d\n", ret);
		return ret;
	}

	/* 2. A sysfs class so udev creates the /dev nodes for us. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	note_class = class_create(CLASS_NAME);
#else
	note_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(note_class)) {
		ret = PTR_ERR(note_class);
		pr_err("kshellnote: class_create failed: %d\n", ret);
		goto err_unregister;
	}

	/* 3. One cdev + device per minor. */
	for (i = 0; i < KSHELLNOTE_COUNT; i++) {
		struct note_dev *d = &note_devs[i];
		dev_t devno = MKDEV(MAJOR(note_base), i);
		struct device *dev;

		mutex_init(&d->lock);
		cdev_init(&d->cdev, &note_fops);
		d->cdev.owner = THIS_MODULE;

		ret = cdev_add(&d->cdev, devno, 1);
		if (ret) {
			pr_err("kshellnote: cdev_add(%d) failed: %d\n", i, ret);
			goto err_devices;
		}

		dev = device_create(note_class, NULL, devno, NULL, DEVICE_NAME "%d", i);
		if (IS_ERR(dev)) {
			ret = PTR_ERR(dev);
			pr_err("kshellnote: device_create(%d) failed: %d\n", i, ret);
			cdev_del(&d->cdev);
			goto err_devices;
		}
	}

	pr_info("kshellnote: loaded, major %d, %d notes (/dev/%s0..%d)\n",
		MAJOR(note_base), KSHELLNOTE_COUNT, DEVICE_NAME, KSHELLNOTE_COUNT - 1);
	return 0;

err_devices:
	note_teardown(i);
	class_destroy(note_class);
err_unregister:
	unregister_chrdev_region(note_base, KSHELLNOTE_COUNT);
	return ret;
}

static void __exit kshellnote_exit(void)
{
	/* Tear down in exactly the reverse order of init. */
	note_teardown(KSHELLNOTE_COUNT);
	class_destroy(note_class);
	unregister_chrdev_region(note_base, KSHELLNOTE_COUNT);
	pr_info("kshellnote: unloaded\n");
}

module_init(kshellnote_init);
module_exit(kshellnote_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Architha Joshi");
MODULE_DESCRIPTION("kshell note buffer character devices");
