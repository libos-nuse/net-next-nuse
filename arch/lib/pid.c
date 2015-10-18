/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <asm/page.h>
#include "sim.h"
#include "sim-assert.h"

struct pid_namespace init_pid_ns;
struct pid *cad_pid = 0;

#define RESERVED_PIDS 300
int pid_max = PID_MAX_DEFAULT;
int pid_max_min = RESERVED_PIDS + 1;
int pid_max_max = PID_MAX_LIMIT;

void put_pid(struct pid *pid)
{
}
pid_t pid_vnr(struct pid *pid)
{
	return pid_nr(pid);
}
struct task_struct *find_task_by_vpid(pid_t nr)
{
	lib_assert(false);
	return 0;
}
struct pid *find_get_pid(int nr)
{
	lib_assert(false);
	return 0;
}
