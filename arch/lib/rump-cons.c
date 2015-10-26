/*
 *  Console driver for rump kernel
 *
 *  Copyright (C) 2015 Hajime Tazaki <thehajime@gmail.com>
 *
 */

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/syscalls.h>

#include "sim-assert.h"

void rumpuser_putchar(int c);

static void rump_console_write(struct console *con, const char *s, unsigned n)
{
	while (n-- > 0) {
		rumpuser_putchar(*s);
		s++;
	}
}

static ssize_t rump_file_write(struct file *fp, const char __user *s,
			       size_t n, loff_t *off)
{
	ssize_t cnt = 0;

	while (n-- > 0) {
		rumpuser_putchar(*s);
		s++;
		cnt++;
	}
	return cnt;
}

static struct console rump_cons_console_dev = {
	.name =		"rump_cons",
	.write =	rump_console_write,
	.flags =	CON_PRINTBUFFER | CON_BOOT,
	.index =	-1,
};

static struct file_operations rump_cons_file_dev = {
	.write =	rump_file_write,
};

void
rump_early_consdev_init(void)
{
	register_console(&rump_cons_console_dev);
}

void
rump_consdev_init(void)
{
	struct file *fp;
	int fd;

	/* open fds 0 */
	if ((__fdget(0) != 0) || (__fdget(1) != 0) ||
	    (__fdget(2) != 0))
		panic("Either of fds 0/1/2 are available\n");

	fd = sys_open("/rump-console", O_CREAT | O_RDWR | O_NDELAY, 0);
	fp = fget(fd);
	fp->f_op = &rump_cons_file_dev;

	if ((sys_dup3(0, 1, 0) == -1) || 
	    (sys_dup3(0, 2, 0) == -1))
		panic("failed to dup fd 0/1/2");
}
