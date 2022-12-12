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

int modules_disabled = 0;
char modprobe_path[KMOD_PATH_LEN] = "/sbin/modprobe";
int watermark_boost_factor;
int watermark_scale_factor;
static struct module_version_attribute g_empty_attr_buffer;
/* we make the array empty by default because, really, we don't need */
/* to look at the builtin params */
const struct kernel_param __start___param[] = {{0}} ;
const struct kernel_param __stop___param[] = {{0}} ;
const struct module_version_attribute *__start___modver[] = {
	&g_empty_attr_buffer};
const struct module_version_attribute *__stop___modver[] = {
	&g_empty_attr_buffer};

struct module_attribute module_uevent;
struct ctl_table usermodehelper_table[] = {};

struct static_key_false init_on_alloc;

int __request_module(bool wait, const char *fmt, ...)
{
	/* we really should never be trying to load modules that way. */
	/*  lib_assert (false); */
	return 0;
}
