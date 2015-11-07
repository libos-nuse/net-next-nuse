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
#include "sim.h"
#include "sim-assert.h"

int dmesg_restrict = 1;

/* from lib/vsprintf.c */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

int printk(const char *fmt, ...)
{
	va_list args;
	static char buf[256];
	int value;

	va_start(args, fmt);
	value = vsnprintf(buf, 256, printk_skip_level(fmt), args);
	lib_printf("<%c>%s", printk_get_level(fmt), buf);
	va_end(args);
	return value;
}
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

