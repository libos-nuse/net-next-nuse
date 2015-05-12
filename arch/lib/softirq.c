/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/interrupt.h>
#include "sim-init.h"
#include "sim.h"
#include "sim-assert.h"


static struct softirq_action softirq_vec[NR_SOFTIRQS];
static struct SimTask *g_softirq_task = 0;
static int g_n_raises = 0;

void lib_softirq_wakeup(void)
{
	g_n_raises++;
	lib_task_wakeup(g_softirq_task);
}

static void softirq_task_function(void *context)
{
	while (true) {
		do_softirq();
		g_n_raises--;
		if (g_n_raises == 0 || local_softirq_pending() == 0) {
			g_n_raises = 0;
			lib_task_wait();
		}
	}
}

static void ensure_task_created(void)
{
	if (g_softirq_task != 0)
		return;
	g_softirq_task = lib_task_start(&softirq_task_function, 0);
}

void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	ensure_task_created();
	softirq_vec[nr].action = action;
}
#define MAX_SOFTIRQ_RESTART 10

void do_softirq(void)
{
	__u32 pending;
	int max_restart = MAX_SOFTIRQ_RESTART;
	struct softirq_action *h;

	pending = local_softirq_pending();

restart:
	/* Reset the pending bitmask before enabling irqs */
	set_softirq_pending(0);

	local_irq_enable();

	h = softirq_vec;

	do {
		if (pending & 1)
			h->action(h);
		h++;
		pending >>= 1;
	} while (pending);

	local_irq_disable();

	pending = local_softirq_pending();
	if (pending && --max_restart)
		goto restart;
}
void raise_softirq_irqoff(unsigned int nr)
{
	__raise_softirq_irqoff(nr);

	lib_softirq_wakeup();
}
void __raise_softirq_irqoff(unsigned int nr)
{
	/* trace_softirq_raise(nr); */
	or_softirq_pending(1UL << nr);
}
int __cond_resched_softirq(void)
{
	/* tell the caller that we did not need to re-schedule. */
	return 0;
}
void raise_softirq(unsigned int nr)
{
	/* copy/paste from kernel/softirq.c */
	unsigned long flags;

	local_irq_save(flags);
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);
}

void __local_bh_enable_ip(unsigned long ip, unsigned int cnt)
{
}
