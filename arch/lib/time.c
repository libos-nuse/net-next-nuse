/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/time.h>
#include <linux/errno.h>
#include <linux/timex.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include "sim.h"
#include "sim-assert.h"

unsigned long volatile jiffies = INITIAL_JIFFIES;

struct timespec xtime;
seqlock_t xtime_lock;
/* accessed from wrap_clock from do_sys_settimeofday.
   We don't call the latter so we should never access this variable. */
struct timespec wall_to_monotonic;

uint64_t ns_to_jiffies(uint64_t ns)
{
	do_div(ns, (1000000000 / HZ));
#ifdef USE_NATIVE_TIMER
	return ns + INITIAL_JIFFIES;
#else
	return ns;
#endif
}

void lib_update_jiffies(void)
{
	uint64_t ns = lib_current_ns();
	jiffies = ns_to_jiffies(ns);
	jiffies_64 = ns_to_jiffies(ns);
	run_local_timers();
}

/* copied from kernel/time/hrtimeer.c */
#if BITS_PER_LONG < 64
/*
 * Divide a ktime value by a nanosecond value
 */
s64 __ktime_divns(const ktime_t kt, s64 div)
{
	int sft = 0;
	s64 dclc;
	u64 tmp;

	dclc = ktime_to_ns(kt);
	tmp = dclc < 0 ? -dclc : dclc;

	/* Make sure the divisor is less than 2^32: */
	while (div >> 32) {
		sft++;
		div >>= 1;
	}
	tmp >>= sft;
	do_div(tmp, (unsigned long) div);
	return dclc < 0 ? -tmp : tmp;
}
#endif /* BITS_PER_LONG >= 64 */

void read_persistent_clock(struct timespec *ts)
{
	u64 nsecs = lib_current_ns();

	set_normalized_timespec(ts, nsecs / NSEC_PER_SEC,
				nsecs % NSEC_PER_SEC);
}

void __init time_init(void)
{
}
