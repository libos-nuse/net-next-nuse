/*
 *  Console driver for rump kernel
 *
 *  Copyright (C) 2015 Hajime Tazaki <thehajime@gmail.com>
 *
 */

#include <linux/kernel.h>
#include <linux/console.h>

#include "sim-assert.h"

/* FIXME: dce should be implement rumpuser(_putchar, etc) */
static void rump_console_dce_write(struct console *con, const char *s, unsigned n)
{
	char buf[256];
	snprintf(buf, n+1, "%s", s);
	lib_printf("%s", buf);
}


static struct console rump_cons_console_dce_dev = {
	.name =		"rump_dce_cons",
	.write =	rump_console_dce_write,
	.flags =	CON_PRINTBUFFER,
	.index =	-1,
};


void
rump_dce_consdev_init(void)
{
	register_console(&rump_cons_console_dce_dev);
}

