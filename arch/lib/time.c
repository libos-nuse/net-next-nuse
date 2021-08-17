#include <linux/time.h>
#include <linux/errno.h>
#include <linux/timex.h>
#include <linux/ktime.h>
#include "sim.h"
#include "sim-assert.h"

unsigned long volatile jiffies = INITIAL_JIFFIES;
u64 jiffies_64 = INITIAL_JIFFIES;

struct timespec64 xtime;
//seqlock_t xtime_lock;
/* accessed from wrap_clock from do_sys_settimeofday.
   We don't call the latter so we should never access this variable. */
struct timespec64 wall_to_monotonic;

uint64_t ns_to_jiffies(uint64_t ns)
{
	do_div(ns, (1000000000 / HZ));
	return ns;
}

void lib_update_jiffies(void)
{
	uint64_t ns = lib_current_ns();
	jiffies = ns_to_jiffies(ns);
	jiffies_64 = ns_to_jiffies(ns);
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

void read_persistent_clock(struct timespec *ts)
{
	u64 nsecs = lib_current_ns();

	set_normalized_timespec(ts, nsecs / NSEC_PER_SEC,
				nsecs % NSEC_PER_SEC);
}

void __init time_init(void)
{
}