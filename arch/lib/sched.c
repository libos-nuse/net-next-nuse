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
#include <linux/fdtable.h>
#include <linux/init_task.h>
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
	task->kernel_task.signal = lib_malloc(sizeof(struct signal_struct));
	task->kernel_task.signal->rlim[RLIMIT_NOFILE].rlim_cur = RLIM_INFINITY;
	task->kernel_task.signal->rlim[RLIMIT_NOFILE].rlim_max = RLIM_INFINITY;
	init_files.resize_in_progress = false;
	init_waitqueue_head(&init_files.resize_wait);
	/* This trick is almost the same as rump hijack library (librumphijack.so) */
#define RUMP_FD_OFFSET 256/2
	init_files.next_fd = RUMP_FD_OFFSET;

	task->kernel_task.files = &init_files,
	task->kernel_task.nsproxy = &init_nsproxy;
	task->kernel_task.fs = &init_fs;

	task->private = private;
	return task;
}
void lib_task_destroy(struct SimTask *task)
{
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


static void wake_trampoline(unsigned long arg)
{
	struct SimTask *lib_task = (struct SimTask *)arg;
	lib_task_wakeup(lib_task);
	lib_task->event = NULL;
}

int default_wake_function(wait_queue_t *curr, unsigned mode, int wake_flags,
			  void *key)
{
	struct task_struct *task = (struct task_struct *)curr->private;
	struct SimTask *lib_task = container_of(task, struct SimTask,
						kernel_task);
	int ret = 1;		/* always notify waking up a task */

	if (!(task->state & mode))
		return 0;

	/* delay wakeup: otherwise local-scoped wait_queue_t corrupted */
#if 0
#if 0
	/* XXX: work-around for blocking signal for pthread_signal()
	 * immediately detach the function. it's still not perfect
	 * work-around...  */
	if (lib_task->event)
		return 1;
	lib_task->event = lib_event_schedule_ns(0, (void *)&wake_trampoline, lib_task);
	/* This block is too sloooooooooooooooooooooow... */
#endif
	struct timer_list timer;
	setup_timer_on_stack(&timer, wake_trampoline, (unsigned long)lib_task);
	mod_timer(&timer, jiffies+1);
	del_singleshot_timer_sync(&timer);
#else
	ret = lib_task_wakeup(lib_task);
#endif
	return ret;
}

void schedule(void)
{
	lib_task_wait();
}


void yield(void)
{
	lib_task_yield();
}

long __sched io_schedule_timeout(long timeout)
{
	return schedule_timeout(timeout);
}

int wake_up_process(struct task_struct *task)
{
	struct SimTask *lib_task =
		container_of(task, struct SimTask, kernel_task);
	int ret;

	ret = lib_task_wakeup(lib_task);
	return ret;
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
