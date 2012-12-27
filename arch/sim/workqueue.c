#include <linux/workqueue.h>
#include <linux/slab.h>
#include "sim.h"
#include "sim-assert.h"

/**
 * All these functions work on the global per-cpu workqueue.
 * Since we model only a single cpu, we have a single 
 * global workqueue.
 */

static LIST_HEAD(g_work);

struct wq_barrier {
  struct work_struct  work;
  struct SimTask      *waiter;
};

/* copy from kernel/workqueue.c */
typedef unsigned long mayday_mask_t;
struct workqueue_struct {
	unsigned int		flags;		/* W: WQ_* flags */
	union {
		struct cpu_workqueue_struct __percpu	*pcpu;
		struct cpu_workqueue_struct		*single;
		unsigned long				v;
	} cpu_wq;				/* I: cwq's */
	struct list_head	list;		/* W: list of all workqueues */

	struct mutex		flush_mutex;	/* protects wq flushing */
	int			work_color;	/* F: current work color */
	int			flush_color;	/* F: current flush color */
	atomic_t		nr_cwqs_to_flush; /* flush in progress */
	struct wq_flusher	*first_flusher;	/* F: first flusher */
	struct list_head	flusher_queue;	/* F: flush waiters */
	struct list_head	flusher_overflow; /* F: flush overflow list */

	mayday_mask_t		mayday_mask;	/* cpus requesting rescue */
	struct worker		*rescuer;	/* I: rescue worker */

	int			nr_drainers;	/* W: drain in progress */
	int			saved_max_active; /* W: saved cwq max_active */
#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif
	char			name[];		/* I: workqueue name */
};

static void workqueue_barrier_fn (struct work_struct *work)
{
  struct wq_barrier *barr = container_of(work, struct wq_barrier, work);
  sim_task_wakeup (barr->waiter);
}

static void
workqueue_function (void *context)
{
  while (true)
    {
      sim_task_wait ();
      while (!list_empty (&g_work))
       {
         struct work_struct *work = list_first_entry(&g_work,
                                                     struct work_struct, entry);
         work_func_t f = work->func;
         __list_del (work->entry.prev, work->entry.next);
         work_clear_pending (work);
         f(work);
       }
    }
}

static struct SimTask *workqueue_task (void)
{
  static struct SimTask *g_task = 0;
  if (g_task == 0)
    {
      g_task = sim_task_start (&workqueue_function, 0);
    }
  return g_task;
}

static int flush_entry (struct list_head *prev)
{
  struct wq_barrier barr;
  int active = 0;

  if (!list_empty(&g_work))
    {
      active = 1;
      INIT_WORK_ONSTACK(&barr.work, &workqueue_barrier_fn);
      __set_bit(WORK_STRUCT_PENDING, work_data_bits(&barr.work));
      list_add(&barr.work.entry, prev);
      sim_task_wakeup (workqueue_task ());
      sim_task_wait ();
    }

  return active;
}

void delayed_work_timer_fn(unsigned long data)
{
  struct delayed_work *dwork = (struct delayed_work *)data;
  struct work_struct *work = &dwork->work;

  schedule_work (work);
}

/**
 * @work: work to queue
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 *
 * We queue the work, the caller must ensure it
 * can't go away.
 */
bool schedule_work(struct work_struct *work)
{
  int ret = 0;

  if (!test_and_set_bit (WORK_STRUCT_PENDING, work_data_bits(work))) {
      list_add_tail (&work->entry, &g_work);
      sim_task_wakeup (workqueue_task ());
      ret = 1;
  }
  return ret;
}
void flush_scheduled_work(void)
{
  flush_entry (g_work.prev);
}
bool flush_work(struct work_struct *work)
{
  return flush_entry (&work->entry);
}
bool cancel_work_sync(struct work_struct *work)
{
  int retval = 0;
  if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work)))
    {
      // work was not yet queued
      return 0;
    }
  if (!list_empty(&work->entry))
    {
      // work was queued. now unqueued.
      list_del_init(&work->entry);
      work_clear_pending (work);
      retval = 1;
    }
  return retval;
}
bool schedule_delayed_work(struct delayed_work *dwork, unsigned long delay)
{
  int ret = 0;
  struct timer_list *timer = &dwork->timer;
  struct work_struct *work = &dwork->work;
  if (delay == 0)
    {
      return schedule_work (work);
    }

  if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work)))
    {
      sim_assert (!timer_pending (timer));
      /* This stores cwq for the moment, for the timer_fn */
      timer->expires = jiffies + delay;
      timer->data = (unsigned long)dwork;
      timer->function = delayed_work_timer_fn;
      add_timer(timer);
      ret = 1;
    }
  return ret;
}
struct workqueue_struct *__alloc_workqueue_key(const char *fmt,
					       unsigned int flags,
					       int max_active,
					       struct lock_class_key *key,
					       const char *lock_name, ...)
{
	va_list args, args1;
	struct workqueue_struct *wq;
	unsigned int cpu;
	size_t namelen;

	/* determine namelen, allocate wq and format name */
	va_start(args, lock_name);
	va_copy(args1, args);
	namelen = vsnprintf(NULL, 0, fmt, args) + 1;

	wq = kzalloc(sizeof(*wq) + namelen, GFP_KERNEL);
	if (!wq)
		goto err;

	vsnprintf(wq->name, namelen, fmt, args1);
	va_end(args);
	va_end(args1);

	/*
	 * Workqueues which may be used during memory reclaim should
	 * have a rescuer to guarantee forward progress.
	 */
	if (flags & WQ_MEM_RECLAIM)
		flags |= WQ_RESCUER;

	max_active = max_active ?: WQ_DFL_ACTIVE;
#if 0
	max_active = wq_clamp_max_active(max_active, flags, wq->name);
#endif
	/* init wq */
	wq->flags = flags;
	wq->saved_max_active = max_active;
	mutex_init(&wq->flush_mutex);
	atomic_set(&wq->nr_cwqs_to_flush, 0);
	INIT_LIST_HEAD(&wq->flusher_queue);
	INIT_LIST_HEAD(&wq->flusher_overflow);

	lockdep_init_map(&wq->lockdep_map, lock_name, key, 0);
	INIT_LIST_HEAD(&wq->list);

#if 0
	if (alloc_cwqs(wq) < 0)
		goto err;

	for_each_cwq_cpu(cpu, wq) {
		struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);
		struct global_cwq *gcwq = get_gcwq(cpu);
		int pool_idx = (bool)(flags & WQ_HIGHPRI);

		BUG_ON((unsigned long)cwq & WORK_STRUCT_FLAG_MASK);
		cwq->pool = &gcwq->pools[pool_idx];
		cwq->wq = wq;
		cwq->flush_color = -1;
		cwq->max_active = max_active;
		INIT_LIST_HEAD(&cwq->delayed_works);
	}

	if (flags & WQ_RESCUER) {
		struct worker *rescuer;

		if (!alloc_mayday_mask(&wq->mayday_mask, GFP_KERNEL))
			goto err;

		wq->rescuer = rescuer = alloc_worker();
		if (!rescuer)
			goto err;

		rescuer->task = kthread_create(rescuer_thread, wq, "%s",
					       wq->name);
		if (IS_ERR(rescuer->task))
			goto err;

		rescuer->task->flags |= PF_THREAD_BOUND;
		wake_up_process(rescuer->task);
	}

	/*
	 * workqueue_lock protects global freeze state and workqueues
	 * list.  Grab it, set max_active accordingly and add the new
	 * workqueue to workqueues list.
	 */
	spin_lock(&workqueue_lock);

	if (workqueue_freezing && wq->flags & WQ_FREEZABLE)
		for_each_cwq_cpu(cpu, wq)
			get_cwq(cpu, wq)->max_active = 0;

	list_add(&wq->list, &workqueues);

	spin_unlock(&workqueue_lock);
#endif

	return wq;
err:
	if (wq) {
#if 0
		free_cwqs(wq);
		free_mayday_mask(wq->mayday_mask);
#endif
		kfree(wq->rescuer);
		kfree(wq);
	}
	return NULL;
}

struct workqueue_struct *system_wq __read_mostly;
static int __init init_workqueues(void)
{
  system_wq = alloc_workqueue("events", 0, 0);
}
