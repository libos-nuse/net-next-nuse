/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/workqueue.h>
#include <linux/slab.h>
#include "sim-assert.h"
#include "sim.h"

typedef unsigned long mayday_mask_t;
struct workqueue_struct {
	unsigned int flags;                     /* W: WQ_* flags */
	union {
		struct cpu_workqueue_struct __percpu *pcpu;
		struct cpu_workqueue_struct *single;
		unsigned long v;
	} cpu_wq;                               /* I: cwq's */
	struct list_head list;                  /* W: list of all workqueues */

	struct mutex flush_mutex;               /* protects wq flushing */
	int work_color;                         /* F: current work color */
	int flush_color;                        /* F: current flush color */
	atomic_t nr_cwqs_to_flush;              /* flush in progress */
	struct wq_flusher *first_flusher;       /* F: first flusher */
	struct list_head flusher_queue;         /* F: flush waiters */
	struct list_head flusher_overflow;      /* F: flush overflow list */

	mayday_mask_t mayday_mask;              /* cpus requesting rescue */
	struct worker *rescuer;                 /* I: rescue worker */

	int nr_drainers;                        /* W: drain in progress */
	int saved_max_active;                   /* W: saved cwq max_active */

	char			*lock_name;
	struct lock_class_key	key;
	struct lockdep_map	lockdep_map;
	
	char name[];                            /* I: workqueue name */
};

struct wq_barrier {
	struct SimTask *waiter;
	struct workqueue_struct wq;
};


static void
workqueue_function(void *context)
{
	
	
	struct workqueue_struct *wq = context;

	while (true) {
		lib_task_wait();
		while (!list_empty(&wq->list)) {
			struct work_struct *work =
				list_first_entry(&wq->list, struct work_struct,
						entry);
			work_func_t f = work->func;

			if (work->entry.prev != LIST_POISON2) {
				list_del_init(&work->entry);
				clear_bit(WORK_STRUCT_PENDING_BIT,
					  work_data_bits(work));
				f(work);
			}
		}
	}
}

static struct SimTask *workqueue_task(struct workqueue_struct *wq)
{
	
	
	struct wq_barrier *barr = container_of(wq, struct wq_barrier, wq);

	if (barr->waiter == 0)
		barr->waiter = lib_task_start(&workqueue_function, wq);
	return barr->waiter;
}

static int flush_entry(struct workqueue_struct *wq, struct list_head *prev)
{
	
	
	int active = 0;

	if (!list_empty(&wq->list)) {
		active = 1;
		lib_task_wakeup(workqueue_task(wq));
		/* XXX: should wait for completion? but this will block
		   and init won't return.. */
		/* lib_task_wait (); */
	}

	return active;
}

//yaah this one
void delayed_work_timer_fn(struct timer_list *data)
{
	
	
	struct delayed_work *dwork = (struct delayed_work *)data;
	struct work_struct *work = &dwork->work;

	list_add_tail(&work->entry, &dwork->wq->list);
	lib_task_wakeup(workqueue_task(dwork->wq));
}

//yaah this one
bool queue_work_on(int cpu, struct workqueue_struct *wq,
		   struct work_struct *work)
{
	
	
	int ret = 0;

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work))) {
		list_add_tail(&work->entry, &wq->list);
		lib_task_wakeup(workqueue_task(wq));
		ret = 1;
	}
	return ret;
}

//yaah this one
bool flush_work(struct work_struct *work)
{
	
	
	return flush_entry(system_wq, &work->entry);
}

//yaah this one
void flush_workqueue(struct workqueue_struct *wq)
{
	
	
	flush_entry(wq, wq->list.prev);
}

bool cancel_work_sync(struct work_struct *work)
{
	
	
	int retval = 0;

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work)))
		
		return 0;
	if (!list_empty(&work->entry)) {
		
		if (work->entry.prev != LIST_POISON2) {
			list_del_init(&work->entry);
			clear_bit(WORK_STRUCT_PENDING_BIT,
				  work_data_bits(work));
			retval = 1;
		}
	}
	return retval;
}

bool queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
			   struct delayed_work *dwork, unsigned long delay)
{
	
	
	int ret = 0;
	struct timer_list *timer = &dwork->timer;
	struct work_struct *work = &dwork->work;

	if (delay == 0)
		return queue_work(wq, work);

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work))) {
		lib_assert(!timer_pending(timer));
		dwork->wq = wq;		
		timer->expires = jiffies + delay;		
		timer->function = delayed_work_timer_fn;
		add_timer(timer);
		ret = 1;
	}
	return ret;
}


bool mod_delayed_work_on(int cpu, struct workqueue_struct *wq,
			 struct delayed_work *dwork, unsigned long delay)
{
	
	
	del_timer(&dwork->timer);
	__clear_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(&dwork->work));
	return queue_delayed_work(wq, dwork, delay);
}

bool cancel_delayed_work(struct delayed_work *dwork)
{
	
	
	del_timer(&dwork->timer);
	return cancel_work_sync(&dwork->work);
}

bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
	return cancel_delayed_work(&dwork);
}

//yaah this one

static void wq_init_lockdep(struct workqueue_struct *wq)
{
	
	
	char *lock_name;

	lockdep_register_key(&wq->key);
	lock_name = kasprintf(GFP_KERNEL, "%s%s", "(wq_completion)", wq->name);
	if (!lock_name)
		lock_name = wq->name;

	wq->lock_name = lock_name;
	lockdep_init_map(&wq->lockdep_map, lock_name, &wq->key, 0);
}

struct workqueue_struct *alloc_workqueue(const char *fmt,
					 unsigned int flags,
					 int max_active, ...)
{
	
	
	va_list args, args1;
	struct wq_barrier *barr;
	struct workqueue_struct *wq;
	size_t namelen;

	
	va_start(args, max_active);
	va_copy(args1, args);
	namelen = vsnprintf(NULL, 0, fmt, args) + 1;

	barr = kzalloc(sizeof(*barr) + namelen, GFP_KERNEL);
	if (!barr)
		goto err;
	barr->waiter = 0;
	wq = &barr->wq;

	vsnprintf(wq->name, namelen, fmt, args1);
	va_end(args);
	va_end(args1);

	max_active = max_active ? : WQ_DFL_ACTIVE;
	
	wq->flags = flags;
	wq->saved_max_active = max_active;
	mutex_init(&wq->flush_mutex);
	atomic_set(&wq->nr_cwqs_to_flush, 0);
	INIT_LIST_HEAD(&wq->flusher_queue);
	INIT_LIST_HEAD(&wq->flusher_overflow);

	wq_init_lockdep(wq);
	INIT_LIST_HEAD(&wq->list);

	
	workqueue_task(wq);
	return wq;
err:
	if (barr)
		kfree(barr);
	return NULL;
}


struct workqueue_struct *system_wq __read_mostly;
struct workqueue_struct *system_power_efficient_wq __read_mostly;
struct workqueue_struct *system_highpri_wq __read_mostly;
struct workqueue_struct *system_long_wq __read_mostly;
struct workqueue_struct *system_unbound_wq __read_mostly;
struct workqueue_struct *system_freezable_wq __read_mostly;
struct workqueue_struct *system_freezable_power_efficient_wq __read_mostly;
// from linux/workqueue.h 
#define system_nrt_wq                   __system_nrt_wq()

static int __init init_workqueues(void)
{
	system_wq = alloc_workqueue("events", 0, 0);
	system_highpri_wq = alloc_workqueue("events_highpri", WQ_HIGHPRI, 0);
	system_long_wq = alloc_workqueue("events_long", 0, 0);
	system_unbound_wq = alloc_workqueue("events_unbound", WQ_UNBOUND,
					    WQ_UNBOUND_MAX_ACTIVE);
	system_freezable_wq = alloc_workqueue("events_freezable",
					      WQ_FREEZABLE, 0);
	system_power_efficient_wq = alloc_workqueue("events_power_efficient",
					      WQ_POWER_EFFICIENT, 0);
	system_freezable_power_efficient_wq = alloc_workqueue("events_freezable_power_efficient",
					      WQ_FREEZABLE | WQ_POWER_EFFICIENT,
					      0);
	return 0;
}
early_initcall(init_workqueues);
