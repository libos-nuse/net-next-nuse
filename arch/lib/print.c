/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <stdarg.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include "sim.h"
#include "sim-assert.h"

/* not used since kernel/printk/printk.c has been used. */
int lib_printk(const char *fmt, ...)
{
	va_list args;
	static char buf[256];
	int value;

	va_start(args, fmt);
	snprintf(buf, 4, "<%c>", printk_get_level(fmt));
	value = vsnprintf(buf+3, 256 - 3, printk_skip_level(fmt), args);
	lib_printf(buf);
	va_end(args);
	return value;
}
#define printk_deferred(X)  printk(X)

void panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	lib_vprintf(fmt, args);
	va_end(args);
	lib_assert(false);
}

void warn_slowpath_fmt(const char *file, int line, const char *fmt, ...)
{
	va_list args;

	printk("%s:%d -- ", file, line);
	va_start(args, fmt);
	lib_vprintf(fmt, args);
	va_end(args);
}

void warn_slowpath_null(const char *file, int line)
{
	printk("%s:%d -- ", file, line);
}

