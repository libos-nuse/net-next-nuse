/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "sim.h"
#include <linux/random.h>
#include <linux/sysctl.h>

static int nothing;
struct ctl_table random_table[] = {
	{
		.procname       = "nothing",
		.data           = &nothing,
		.maxlen         = sizeof(int),
		.mode           = 0444,
		.proc_handler   = proc_dointvec,
	},
	{}
};

void get_random_bytes(void *buf, int nbytes)
{
	char *p = (char *)buf;
	int i;

	for (i = 0; i < nbytes; i++)
		p[i] = lib_random();
}
