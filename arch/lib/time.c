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
#include "sim.h"
#include "sim-assert.h"

unsigned long volatile jiffies = INITIAL_JIFFIES;
u64 jiffies_64 = INITIAL_JIFFIES;

struct timespec xtime;
seqlock_t xtime_lock;
/* accessed from wrap_clock from do_sys_settimeofday.
   We don't call the latter so we should never access this variable. */
struct timespec wall_to_monotonic;

uint64_t ns_to_jiffies(uint64_t ns)
{
	do_div(ns, (1000000000 / HZ));
	return ns;
}

void lib_update_jiffies(void)
{
	jiffies = ns_to_jiffies(lib_current_ns());
	jiffies_64 = ns_to_jiffies(lib_current_ns());
}

struct timespec current_kernel_time(void)
{
	u64 ns = lib_current_ns();
	struct timespec spec = ns_to_timespec(ns);

	return spec;
}

void do_gettimeofday(struct timeval *tv)
{
	u64 ns = lib_current_ns();

	*tv = ns_to_timeval(ns);
}

int do_adjtimex(struct timex *timex)
{
	lib_assert(false);
	return -EPERM;
}
ktime_t ktime_get(void)
{
	u64 ns = lib_current_ns();

	return ns_to_ktime(ns);
}
ktime_t ktime_get_with_offset(enum tk_offsets offs)
{
	/* FIXME */
	return ktime_get();
}

/* copied from kernel/time/hrtimeer.c */
#if BITS_PER_LONG < 64
/*
 * Divide a ktime value by a nanosecond value
 */
u64 __ktime_divns(const ktime_t kt, s64 div)
{
	u64 dclc;
	int sft = 0;

	dclc = ktime_to_ns(kt);
	/* Make sure the divisor is less than 2^32: */
	while (div >> 32) {
		sft++;
		div >>= 1;
	}
	dclc >>= sft;
	do_div(dclc, (unsigned long)div);

	return dclc;
}
#endif /* BITS_PER_LONG >= 64 */

void update_xtime_cache(u64 nsec)
{
}
unsigned long get_seconds(void)
{
	u64 ns = lib_current_ns();

	do_div(ns, 1000000000);
	return ns;
}
static unsigned long
round_jiffies_common(unsigned long j,
		     bool force_up)
{
	int rem;
	unsigned long original = j;

	rem = j % HZ;
	if (rem < HZ / 4 && !force_up)  /* round down */
		j = j - rem;
	else                            /* round up */
		j = j - rem + HZ;
	if (j <= jiffies)               /* rounding ate our timeout entirely; */
		return original;
	return j;
}
unsigned long round_jiffies(unsigned long j)
{
	return round_jiffies_common(j, false);
}
unsigned long round_jiffies_relative(unsigned long j)
{
	unsigned long j0 = jiffies;

	/* Use j0 because jiffies might change while we run */
	return round_jiffies_common(j + j0, false) - j0;
}
unsigned long round_jiffies_up(unsigned long j)
{
	return round_jiffies_common(j, true);
}
static void msleep_trampoline(void *context)
{
	struct SimTask *task = context;

	lib_task_wakeup(task);
}
void msleep(unsigned int msecs)
{
	lib_event_schedule_ns(((__u64)msecs) * 1000000, &msleep_trampoline,
			      lib_task_current());
	lib_task_wait();
}
