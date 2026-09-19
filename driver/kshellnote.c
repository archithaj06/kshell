// SPDX-License-Identifier: GPL-2.0
/*
 * kshellnote - minimal kernel module.
 *
 * Phase 2: prove the toolchain. Just log on load and unload.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init kshellnote_init(void)
{
	pr_info("kshellnote: loaded\n");
	return 0;
}

static void __exit kshellnote_exit(void)
{
	pr_info("kshellnote: unloaded\n");
}

module_init(kshellnote_init);
module_exit(kshellnote_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hemanth Simhadri");
MODULE_DESCRIPTION("kshell note buffer character device");
