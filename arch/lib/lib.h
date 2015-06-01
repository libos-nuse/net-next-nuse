/*
 * library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#ifndef LIB_H
#define LIB_H

#include <linux/sched.h>

struct SimTask {
	struct list_head head;
	struct task_struct kernel_task;
	void *event;
	void *private;
};

#ifdef CONFIG_HAVE_ARCH_PFN_VALID
extern int memblock_is_memory(phys_addr_t addr);
#endif

#endif /* LIB_H */
