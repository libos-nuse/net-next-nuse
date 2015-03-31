/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#include "sim.h"
#include "sim-assert.h"
#include <linux/fs.h>


ssize_t generic_file_aio_read(struct kiocb *a, const struct iovec *b,
			      unsigned long c, loff_t d)
{
	lib_assert(false);

	return 0;
}

int generic_file_readonly_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -ENOSYS;
}

ssize_t
generic_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	return 0;
}
