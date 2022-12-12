/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/hrtimer.h>
#include "sim-assert.h"
#include "sim.h"

/**
 * hrtimer_init - initialize a timer to the given clock
 * @timer:      the timer to be initialized
 * @clock_id:   the clock to be used
 * @mode:       timer mode abs/rel
 */
void hrtimer_init(struct hrtimer *timer, clockid_t clock_id,
		  enum hrtimer_mode mode)
{
	memset(timer, 0, sizeof(*timer));
}
static void trampoline(void *context)
{
	struct hrtimer *timer = context;
	enum hrtimer_restart restart = timer->function(timer);

	if (restart == HRTIMER_RESTART) {
		void *event =
			lib_event_schedule_ns(ktime_to_ns(timer->_softexpires),
					      &trampoline, timer);
		timer->base = event;
	} else {
		/* mark as completed. */
		timer->base = 0;
	}
}
/**
 * hrtimer_start_range_ns - (re)start an hrtimer on the current CPU
 * @timer:      the timer to be added
 * @tim:        expiry time
 * @delta_ns:   "slack" range for the timer
 * @mode:       expiry mode: absolute (HRTIMER_ABS) or relative (HRTIMER_REL)
 *
 * Returns:
 *  0 on success
 *  1 when the timer was active
 */
int __hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
			     unsigned long delta_ns,
			     const enum hrtimer_mode mode,
			     int wakeup)
{
	int ret = hrtimer_cancel(timer);
	s64 ns = ktime_to_ns(tim);
	void *event;

	if (mode == HRTIMER_MODE_ABS)
		ns -= lib_current_ns();
	timer->_softexpires = ns_to_ktime(ns);
	event = lib_event_schedule_ns(ns, &trampoline, timer);
	timer->base = event;
	return ret;
}
/**
 * hrtimer_try_to_cancel - try to deactivate a timer
 * @timer:      hrtimer to stop
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 * -1 when the timer is currently excuting the callback function and
 *    cannot be stopped
 */
int hrtimer_try_to_cancel(struct hrtimer *timer)
{
	/* Note: we cannot return -1 from this function.
	   see comment in hrtimer_cancel. */
	if (timer->base == 0)
		/* timer was not active yet */
		return 1;
	lib_event_cancel(timer->base);
	timer->base = 0;
	return 0;
}
/**
 * hrtimer_cancel - cancel a timer and wait for the handler to finish.
 * @timer:      the timer to be cancelled
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 */
int hrtimer_cancel(struct hrtimer *timer)
{
	/* Note: because we assume a uniprocessor non-interruptible */
	/* system when running in the kernel, we know that the timer */
	/* is not running when we execute this code, so, know that */
	/* try_to_cancel cannot return -1 and we don't need to retry */
	/* the cancel later to wait for the handler to finish. */
	int ret = hrtimer_try_to_cancel(timer);

	lib_assert(ret >= 0);
	return ret;
}
void hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
				   u64 range_ns, const enum hrtimer_mode mode)
{
	__hrtimer_start_range_ns(timer, tim, range_ns, mode, 1);
}

int hrtimer_get_res(const clockid_t which_clock, struct timespec64 *tp)
{
	*tp = ns_to_timespec64(1);
	return 0;
}
