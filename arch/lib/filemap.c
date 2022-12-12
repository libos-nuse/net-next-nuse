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
