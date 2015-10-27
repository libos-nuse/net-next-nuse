/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#include "sim-assert.h"
#include <linux/moduleparam.h>
#include <linux/kmod.h>
#include <linux/module.h>


static struct module_version_attribute g_empty_attr_buffer;
/* we make the array empty by default because, really, we don't need */
/* to look at the builtin params */
const struct kernel_param __start___param[] = {{0}} ;
const struct kernel_param __stop___param[] = {{0}} ;
const struct module_version_attribute *__start___modver[] = {
	&g_empty_attr_buffer};
const struct module_version_attribute *__stop___modver[] = {
	&g_empty_attr_buffer};
