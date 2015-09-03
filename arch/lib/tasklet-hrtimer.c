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

static enum hrtimer_restart __hrtimer_tasklet_trampoline(struct hrtimer *timer)
{
	struct tasklet_hrtimer *ttimer =
		container_of(timer, struct tasklet_hrtimer, timer);

	tasklet_schedule(&ttimer->tasklet);
	return HRTIMER_NORESTART;
}
static void __tasklet_hrtimer_trampoline(unsigned long data)
{
	struct tasklet_hrtimer *ttimer = (void *)data;
	enum hrtimer_restart restart;

	restart = ttimer->function(&ttimer->timer);
	if (restart != HRTIMER_NORESTART)
		hrtimer_restart(&ttimer->timer);
}
/**
 * tasklet_hrtimer_init - Init a tasklet/hrtimer combo for softirq callbacks
 * @ttimer:      tasklet_hrtimer which is initialized
 * @function:    hrtimer callback function which gets called from softirq context
 * @which_clock: clock id (CLOCK_MONOTONIC/CLOCK_REALTIME)
 * @mode:        hrtimer mode (HRTIMER_MODE_ABS/HRTIMER_MODE_REL)
 */
void tasklet_hrtimer_init(struct tasklet_hrtimer *ttimer,
			  enum hrtimer_restart (*function)(struct hrtimer *),
			  clockid_t which_clock, enum hrtimer_mode mode)
{
	hrtimer_init(&ttimer->timer, which_clock, mode);
	ttimer->timer.function = __hrtimer_tasklet_trampoline;
	tasklet_init(&ttimer->tasklet, __tasklet_hrtimer_trampoline,
		     (unsigned long)ttimer);
	ttimer->function = function;
}

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
