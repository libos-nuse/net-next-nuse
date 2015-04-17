/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/wait.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>
#include <linux/hash.h>
#include <net/net_namespace.h>
#include "lib.h"
#include "sim.h"
#include "sim-assert.h"

/**
   called by wait_event macro:
   - prepare_to_wait
   - schedule
   - finish_wait
 */

struct SimTask *lib_task_create(void *private, unsigned long pid)
{
	struct SimTask *task = lib_malloc(sizeof(struct SimTask));
	struct cred *cred;
	struct nsproxy *ns;
	struct user_struct *user;
	struct thread_info *info;
	struct pid *kpid;

	if (!task)
		return NULL;
	memset(task, 0, sizeof(struct SimTask));
	cred = lib_malloc(sizeof(struct cred));
	if (!cred)
		return NULL;
	/* XXX: we could optimize away this allocation by sharing it
	   for all tasks */
	ns = lib_malloc(sizeof(struct nsproxy));
	if (!ns)
		return NULL;
	user = lib_malloc(sizeof(struct user_struct));
	if (!user)
		return NULL;
	info = alloc_thread_info(&task->kernel_task);
	if (!info)
		return NULL;
	kpid = lib_malloc(sizeof(struct pid));
	if (!kpid)
		return NULL;
	kpid->numbers[0].nr = pid;
	cred->fsuid = make_kuid(current_user_ns(), 0);
	cred->fsgid = make_kgid(current_user_ns(), 0);
	cred->user = user;
	atomic_set(&cred->usage, 1);
	info->task = &task->kernel_task;
	info->preempt_count = 0;
	info->flags = 0;
	atomic_set(&ns->count, 1);
	ns->uts_ns = 0;
	ns->ipc_ns = 0;
	ns->mnt_ns = 0;
	ns->pid_ns_for_children = 0;
	ns->net_ns = &init_net;
	task->kernel_task.cred = cred;
	task->kernel_task.pid = pid;
	task->kernel_task.pids[PIDTYPE_PID].pid = kpid;
	task->kernel_task.pids[PIDTYPE_PGID].pid = kpid;
	task->kernel_task.pids[PIDTYPE_SID].pid = kpid;
	task->kernel_task.nsproxy = ns;
	task->kernel_task.stack = info;
	/* this is a hack. */
	task->kernel_task.group_leader = &task->kernel_task;
	task->private = private;
	return task;
}
void lib_task_destroy(struct SimTask *task)
{
	lib_free((void *)task->kernel_task.nsproxy);
	lib_free((void *)task->kernel_task.cred);
	lib_free((void *)task->kernel_task.cred->user);
	free_thread_info(task->kernel_task.stack);
	lib_free(task);
}
void *lib_task_get_private(struct SimTask *task)
{
	return task->private;
}

int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	struct SimTask *task = lib_task_start((void (*)(void *))fn, arg);

	return task->kernel_task.pid;
}

struct task_struct *get_current(void)
{
	struct SimTask *lib_task = lib_task_current();

	return &lib_task->kernel_task;
}

struct thread_info *current_thread_info(void)
{
	return task_thread_info(get_current());
}
struct thread_info *alloc_thread_info(struct task_struct *task)
{
	return lib_malloc(sizeof(struct thread_info));
}
void free_thread_info(struct thread_info *ti)
{
	lib_free(ti);
}


void __put_task_struct(struct task_struct *t)
{
	lib_free(t);
}

void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	list_add(&wait->task_list, &q->task_list);
}
void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait)
{
	wait->flags |= WQ_FLAG_EXCLUSIVE;
	list_add_tail(&wait->task_list, &q->task_list);
}
void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	if (wait->task_list.prev != LIST_POISON2)
		list_del(&wait->task_list);
}
void
prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	wait->flags |= WQ_FLAG_EXCLUSIVE;
	if (list_empty(&wait->task_list))
		list_add_tail(&wait->task_list, &q->task_list);
	set_current_state(state);
}
void prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	unsigned long flags;

	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	if (list_empty(&wait->task_list))
		__add_wait_queue(q, wait);
	set_current_state(state);
	spin_unlock_irqrestore(&q->lock, flags);
}
void finish_wait(wait_queue_head_t *q, wait_queue_t *wait)
{
	set_current_state(TASK_RUNNING);
	if (!list_empty(&wait->task_list))
		list_del_init(&wait->task_list);
}
int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync,
			     void *key)
{
	int ret = default_wake_function(wait, mode, sync, key);

	if (ret && (wait->task_list.prev != LIST_POISON2))
		list_del_init(&wait->task_list);

	return ret;
}

int woken_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	wait->flags |= WQ_FLAG_WOKEN;
	return default_wake_function(wait, mode, sync, key);
}

void __init_waitqueue_head(wait_queue_head_t *q, const char *name,
			   struct lock_class_key *k)
{
	INIT_LIST_HEAD(&q->task_list);
}
/**
 * wait_for_completion: - waits for completion of a task
 * @x:  holds the state of this particular completion
 *
 * This waits to be signaled for completion of a specific task. It is NOT
 * interruptible and there is no timeout.
 *
 * See also similar routines (i.e. wait_for_completion_timeout()) with timeout
 * and interrupt capability. Also see complete().
 */
void wait_for_completion(struct completion *x)
{
	wait_for_completion_timeout(x, MAX_SCHEDULE_TIMEOUT);
}
unsigned long wait_for_completion_timeout(struct completion *x,
					  unsigned long timeout)
{
	if (!x->done) {
		DECLARE_WAITQUEUE(wait, current);
		set_current_state(TASK_UNINTERRUPTIBLE);
		wait.flags |= WQ_FLAG_EXCLUSIVE;
		list_add_tail(&wait.task_list, &x->wait.task_list);
		do
			timeout = schedule_timeout(timeout);
		while (!x->done && timeout);
		if (wait.task_list.prev != LIST_POISON2)
			list_del(&wait.task_list);

		if (!x->done)
			return timeout;
	}
	x->done--;
	return timeout ? : 1;
}

/**
 * __wake_up - wake up threads blocked on a waitqueue.
 * @q: the waitqueue
 * @mode: which threads
 * @nr_exclusive: how many wake-one or wake-many threads to wake up
 * @key: is directly passed to the wakeup function
 *
 * It may be assumed that this function implies a write memory barrier before
 * changing the task state if and only if any tasks are woken up.
 */
void __wake_up(wait_queue_head_t *q, unsigned int mode,
	       int nr_exclusive, void *key)
{
	wait_queue_t *curr, *next;

	list_for_each_entry_safe(curr, next, &q->task_list, task_list) {
		unsigned flags = curr->flags;

		if (curr->func(curr, mode, 0, key) &&
		    (flags & WQ_FLAG_EXCLUSIVE) &&
		    !--nr_exclusive)
			break;
	}
}
void __wake_up_sync_key(wait_queue_head_t *q, unsigned int mode,
			int nr_exclusive, void *key)
{
	__wake_up(q, mode, nr_exclusive, key);
}
int default_wake_function(wait_queue_t *curr, unsigned mode, int wake_flags,
			  void *key)
{
	struct task_struct *task = (struct task_struct *)curr->private;
	struct SimTask *lib_task = container_of(task, struct SimTask,
						kernel_task);

	return lib_task_wakeup(lib_task);
}
__sched int bit_wait(struct wait_bit_key *word)
{
	if (signal_pending_state(current->state, current))
		return 1;
	schedule();
	return 0;
}
int wake_bit_function(wait_queue_t *wait, unsigned mode, int sync, void *arg)
{
	struct wait_bit_key *key = arg;
	struct wait_bit_queue *wait_bit
		= container_of(wait, struct wait_bit_queue, wait);

	if (wait_bit->key.flags != key->flags ||
			wait_bit->key.bit_nr != key->bit_nr ||
			test_bit(key->bit_nr, key->flags))
		return 0;
	else
		return autoremove_wake_function(wait, mode, sync, key);
}
void __wake_up_bit(wait_queue_head_t *wq, void *word, int bit)
{
	struct wait_bit_key key = __WAIT_BIT_KEY_INITIALIZER(word, bit);
	if (waitqueue_active(wq))
		__wake_up(wq, TASK_NORMAL, 1, &key);
}
void wake_up_bit(void *word, int bit)
{
	/* FIXME */
	return;
	__wake_up_bit(bit_waitqueue(word, bit), word, bit);
}
wait_queue_head_t *bit_waitqueue(void *word, int bit)
{
	const int shift = BITS_PER_LONG == 32 ? 5 : 6;
	const struct zone *zone = page_zone(virt_to_page(word));
	unsigned long val = (unsigned long)word << shift | bit;

	return &zone->wait_table[hash_long(val, zone->wait_table_bits)];
}


void schedule(void)
{
	lib_task_wait();
}

static void trampoline(void *context)
{
	struct SimTask *task = context;

	lib_task_wakeup(task);
}

signed long schedule_timeout(signed long timeout)
{
	u64 ns;
	struct SimTask *self;

	if (timeout == MAX_SCHEDULE_TIMEOUT) {
		lib_task_wait();
		return MAX_SCHEDULE_TIMEOUT;
	}
	lib_assert(timeout >= 0);
	ns = ((__u64)timeout) * (1000000000 / HZ);
	self = lib_task_current();
	lib_event_schedule_ns(ns, &trampoline, self);
	lib_task_wait();
	/* we know that we are always perfectly on time. */
	return 0;
}

signed long schedule_timeout_uninterruptible(signed long timeout)
{
	return schedule_timeout(timeout);
}
signed long schedule_timeout_interruptible(signed long timeout)
{
	return schedule_timeout(timeout);
}

void yield(void)
{
	lib_task_yield();
}

void complete_all(struct completion *x)
{
	x->done += UINT_MAX / 2;
	__wake_up(&x->wait, TASK_NORMAL, 0, 0);
}
void complete(struct completion *x)
{
	x->done++;
	__wake_up(&x->wait, TASK_NORMAL, 1, 0);
}

long wait_for_completion_interruptible_timeout(
	struct completion *x, unsigned long timeout)
{
	return wait_for_completion_timeout(x, timeout);
}
int wait_for_completion_interruptible(struct completion *x)
{
	wait_for_completion_timeout(x, MAX_SCHEDULE_TIMEOUT);
	return 0;
}
int wake_up_process(struct task_struct *tsk)
{
	struct SimTask *lib_task =
		container_of(tsk, struct SimTask, kernel_task);

	return lib_task_wakeup(lib_task);
}
int _cond_resched(void)
{
	/* we never schedule to decrease latency. */
	return 0;
}
int idle_cpu(int cpu)
{
	/* we are never idle: we call this from rcutiny.c and the answer */
	/* does not matter, really. */
	return 0;
}

unsigned long long __attribute__((weak)) sched_clock(void)
{
	return (unsigned long long)(jiffies - INITIAL_JIFFIES)
	       * (NSEC_PER_SEC / HZ);
}

u64 local_clock(void)
{
	return sched_clock();
}

void __sched schedule_preempt_disabled(void)
{
}

void resched_cpu(int cpu)
{
	rcu_sched_qs();
}
