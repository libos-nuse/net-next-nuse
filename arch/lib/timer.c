/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/hash.h>
#include <linux/hashtable.h>

#include "sim-assert.h"
#include "sim.h"

static DEFINE_HASHTABLE(lib_timers_hashtable, 9);
struct lib_timer {
	struct timer_list *timer;
	struct hlist_node t_hash;
	void *event;
};

static int hash(struct timer_list *timer)
{
	return hash_32(hash32_ptr(timer), HASH_BITS(lib_timers_hashtable));
}

static struct lib_timer *__lib_timers_find(struct hlist_head *head,
					   struct timer_list *timer)
{
	struct lib_timer *l_timer;

	hlist_for_each_entry(l_timer, head, t_hash) {
		if (l_timer->timer == timer)
			return l_timer;
	}
	return NULL;
}

static struct lib_timer *lib_timer_find(struct timer_list *timer)
{
	struct hlist_head *head;

	head = &lib_timers_hashtable[hash(timer)];
	return __lib_timers_find(head, timer);
}

/**
 * init_timer_key - initialize a timer
 * @timer: the timer to be initialized
 * @name: name of the timer
 * @key: lockdep class key of the fake lock used for tracking timer
 *       sync lock dependencies
 *
 * init_timer_key() must be done to a timer prior calling *any* of the
 * other timer functions.
 */
void init_timer_key(struct timer_list *timer,
		    void (*func)(struct timer_list *), unsigned int flags,
		    const char *name, struct lock_class_key *key)
{
	/**
	 * Note: name and key are used for debugging. We ignore them
	 * unconditionally.
	 * Note: we do not initialize the lockdep map either because we
	 * don't care.
	 * and, finally, we never care about the base field either.
	 *
	 * So, for now, we have a timer which is marked as "not started"
	 * thanks to its entry.pprev field set to NULL (timer_pending
	 * will return 0)
	 */
	timer->function = func;
	timer->entry.pprev = NULL;
	if (WARN_ON_ONCE(flags & ~TIMER_INIT_FLAGS))
		flags &= TIMER_INIT_FLAGS;
	timer->flags = flags;
}

struct hlist_head g_expired_events;
struct hlist_head g_pending_events;

void run_timer_softirq(struct softirq_action *h)
{	
	while (!hlist_empty(&g_expired_events)) {		
		struct timer_list *timer =
			hlist_entry((&g_expired_events)->first,
				    struct timer_list, entry);
		void (*fn)(struct timer_list *) = NULL;
		unsigned long data;
		struct lib_timer *l_timer = lib_timer_find(timer);		
		if(timer->function)
		fn = timer->function;			
		lib_assert(l_timer->event == 0);		
		if (l_timer->t_hash.next != LIST_POISON1) {			
			hlist_del(&l_timer->t_hash);
			lib_free(l_timer);
		}		
		if (timer->entry.next != LIST_POISON1){			
			hlist_del(&timer->entry);			
		}		
		timer->entry.pprev = NULL;		
		fn(timer);		
	}
}

static void ensure_softirq_opened(void)
{
	static bool opened = false;

	if (opened)
		return;
	opened = true;
	open_softirq(TIMER_SOFTIRQ, run_timer_softirq);
}
static void timer_trampoline(void *context)
{
	struct timer_list *timer;
	struct lib_timer *l_timer;

	ensure_softirq_opened();
	timer = context;

	if (timer->entry.next != LIST_POISON1)
		hlist_del(&timer->entry);

	l_timer = lib_timer_find(timer);
	/* canceled */
	if (!l_timer)
		return;
	l_timer->event = NULL;

	timer->entry.pprev = NULL;
	hlist_add_head(&timer->entry, &g_expired_events);
	raise_softirq(TIMER_SOFTIRQ);
}
/**
 * add_timer - start a timer
 * @timer: the timer to be added
 *
 * The kernel will do a ->function(->data) callback from the
 * timer interrupt at the ->expires point in the future. The
 * current time is 'jiffies'.
 *
 * The timer's ->expires, ->function (and if the handler uses it, ->data)
 * fields must be set prior calling this function.
 *
 * Timers with an ->expires field in the past will be executed in the next
 * timer tick.
 */
#define LONG_MAX	((long)(~0UL >> 1))
#define ULONG_MAX	(~0UL)
#define LLONG_MAX	((long long)(~0ULL >> 1))

void add_timer(struct timer_list *timer)
{
	__u64 delay_ns = 0;
	struct hlist_head *head;

	lib_assert(!timer_pending(timer));
	if (timer->expires <= jiffies)
		delay_ns = (1000000000 / HZ); /* next tick. */
	else
		delay_ns =
			((__u64)timer->expires *
			 (1000000000 / HZ)) - lib_current_ns();
		
	void *event = lib_event_schedule_ns(delay_ns, &timer_trampoline, timer);

	/* store the external event in the hash table */
	/* to be able to retrieve it from del_timer */
	head = &lib_timers_hashtable[hash(timer)];
	if (!__lib_timers_find(head, timer)) {
		struct lib_timer *l_timer;

		l_timer = lib_malloc(sizeof(struct lib_timer));
		memset(l_timer, 0, sizeof(struct lib_timer));
		l_timer->timer = timer;
		l_timer->event = event;
		hlist_add_head(&l_timer->t_hash, head);
	}

	/* finally, store timer in list of pending events. */
	hlist_add_head(&timer->entry, &g_pending_events);
}
/**
 * del_timer - deactive a timer.
 * @timer: the timer to be deactivated
 *
 * del_timer() deactivates a timer - this works on both active and inactive
 * timers.
 *
 * The function returns whether it has deactivated a pending timer or not.
 * (ie. del_timer() of an inactive timer returns 0, del_timer() of an
 * active timer returns 1.)
 */
int del_timer(struct timer_list *timer)
{
	int retval;
	struct lib_timer *l_timer;

	if (timer->entry.pprev == NULL)
		return 0;

	l_timer = lib_timer_find(timer);
	if (l_timer != NULL && l_timer->event != NULL) {
		lib_event_cancel(l_timer->event);

		if (l_timer->t_hash.next != LIST_POISON1) {
			hlist_del(&l_timer->t_hash);
			lib_free(l_timer);
		}
		retval = 1;
	} else {
		retval = 0;
	}

	if (timer->entry.next != LIST_POISON1)
		hlist_del(&timer->entry);
	timer->entry.pprev = NULL;
	return retval;
}

/* ////////////////////// */

/*void init_timer_deferrable_key(struct timer_list *timer,
			       const char *name,
			       struct lock_class_key *key)
{
	init_timer_key(timer, 0, name, key);
}*/

/**
 * add_timer_on - start a timer on a particular CPU
 * @timer: the timer to be added
 * @cpu: the CPU to start it on
 *
 * This is not very scalable on SMP. Double adds are not possible.
 */
void add_timer_on(struct timer_list *timer, int cpu)
{
	/* we ignore the cpu: we have only one. */
	add_timer(timer);
}
/**
 * mod_timer - modify a timer's timeout
 * @timer: the timer to be modified
 * @expires: new timeout in jiffies
 *
 * mod_timer() is a more efficient way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 *
 * mod_timer(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 *
 * Note that if there are multiple unserialized concurrent users of the
 * same timer, then mod_timer() is the only safe way to modify the timeout,
 * since add_timer() cannot modify an already running timer.
 *
 * The function returns whether it has modified a pending timer or not.
 * (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
 * active timer returns 1.)
 */
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	int ret;

	/* common optimization stolen from kernel */
	if (timer_pending(timer) && timer->expires == expires)
		return 1;

	ret = del_timer(timer);
	timer->expires = expires;	
	add_timer(timer);
	return ret;
}
/**
 * mod_timer_pending - modify a pending timer's timeout
 * @timer: the pending timer to be modified
 * @expires: new timeout in jiffies
 *
 * mod_timer_pending() is the same for pending timers as mod_timer(),
 * but will not re-activate and modify already deleted timers.
 *
 * It is useful for unserialized use of timers.
 */
int mod_timer_pending(struct timer_list *timer, unsigned long expires)
{
	if (timer_pending(timer))
		return 0;
	return mod_timer(timer, expires);
}

int mod_timer_pinned(struct timer_list *timer, unsigned long expires)
{
	if (timer->expires == expires && timer_pending(timer))
		return 1;

	return mod_timer(timer, expires);
}

int timer_reduce(struct timer_list *timer, unsigned long expires)
{
	printk("HAmlo red rimer");
	unsigned long diff = timer->expires-expires;
	if(!diff || diff <= 0) return 1;

	return mod_timer(timer, expires);
}