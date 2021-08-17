/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/interrupt.h>
#include "sim.h"
#include "sim-assert.h"

void __tasklet_hi_schedule(struct tasklet_struct *t)
{
	/* Note: no need to set TASKLET_STATE_SCHED because
	   it is set by caller. */
	lib_assert(t->next == 0);
	/* run the tasklet at the next immediately available opportunity. */
	void *event =
		lib_event_schedule_ns(0, (void *)&t->func, (void *)t->data);
	t->next = event;
}

