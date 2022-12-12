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
#include <linux/sched/signal.h>
#include <linux/nsproxy.h>
#include <linux/hash.h>
#include <linux/init_task.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/fdtable.h>
#include "lib.h"
#include "sim.h"
#include "sim-assert.h"

#define __sched		__attribute__((__section__(".sched.text")))

extern struct net init_ns;
#define WAITQUEUE_WALK_BREAK_CNT 64

int copy_fs(unsigned long clone_flags, struct task_struct *tsk);

/**
   called by wait_event macro:
   - prepare_to_wait
   - schedule
   - finish_wait
 */

void lib_rcu_qs(void){
	
	
	rcu_qs();
}

void lib_rcu_idle_enter(void){
	
	
	rcu_idle_enter();
}

void lib_rcu_idle_exit(void){
	
	
	rcu_idle_exit();
}

struct thread_info *alloc_thread_info(struct task_struct *task)
{
	
	
	return lib_malloc(sizeof(struct thread_info));
}

void free_thread_info(struct thread_info *ti)
{
	
	lib_free(ti);
}

extern struct path def_root;
extern struct mnt_namespace * def_mnt_ns;
struct fdtable * alloc_fdtable(unsigned int nr);

struct fdtable * COPY_FDTABLE = NULL;

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
	
	cred->euid =  init_cred.euid;
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
	ns->mnt_ns = def_mnt_ns;
	ns->pid_ns_for_children = 0;	
	
	ns->net_ns = &init_net;	
	
	task->kernel_task.cred = cred;
	task->kernel_task.pid = pid;
	task->kernel_task.thread_pid = kpid;	
	
	task->kernel_task.signal = lib_malloc(sizeof(struct signal_struct));
	task->kernel_task.signal->pids[PIDTYPE_PGID] = kpid;
	task->kernel_task.signal->pids[PIDTYPE_SID] = kpid;
	task->kernel_task.signal->pids[PIDTYPE_TGID] = kpid;
	// File open Hack
	task->kernel_task.signal->rlim[RLIMIT_NOFILE].rlim_cur = NR_OPEN_DEFAULT;
	task->kernel_task.signal->rlim[RLIMIT_NOFILE].rlim_max = NR_OPEN_DEFAULT;
	
	task->kernel_task.nsproxy = ns;
	
	task->kernel_task.stack = info;
	
	/* this is a hack. */
	task->kernel_task.group_leader = &task->kernel_task;	
	task->private = private;
	//task->kernel_task.fs = copy_fs_struct(&init_fs);	
	struct fs_struct *fs = lib_malloc(sizeof(struct fs_struct));
	fs->users = 1;
	fs->in_exec=0;
	spin_lock_init(&fs->lock);        
	seqcount_spinlock_init(&fs->seq, &fs->lock);	        
    fs->umask = 777;
	fs->root = def_root;
	fs->pwd = def_root;
	task->kernel_task.fs = fs;

	
	struct files_struct * files;
	struct fdtable *  fdt;
	files = kmem_cache_alloc(files_cachep, GFP_KERNEL);	

	//if(COPY_FDTABLE == NULL)
	//COPY_FDTABLE = alloc_fdtable(NR_OPEN_DEFAULT);	

	//fdt = COPY_FDTABLE;
	fdt = alloc_fdtable(NR_OPEN_DEFAULT);
	
	atomic_set(&files->count, 1);
	files->next_fd = 0;
	init_waitqueue_head(&files->resize_wait);
	spin_lock_init(&files->file_lock);
	files->fdt = fdt;	
	task->kernel_task.files = files;	

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

// yaah this one
int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{	
	struct SimTask *task = lib_task_start((void (*)(void *))fn, arg);
	return task->kernel_task.pid;
}

// yaah this one
struct task_struct *get_current(void)
{	
	struct SimTask *lib_task = lib_task_current();	
	return &lib_task->kernel_task;
}

// yaah this one
struct thread_info *current_thread_info(void)
{		
	return task_thread_info(get_current());
}


void __put_task_struct(struct task_struct *t)
{
	
	
	lib_free(t);
}

void add_wait_queue(wait_queue_head_t *q, wait_queue_entry_t *wait)
{
	
	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	list_add(&wait->entry, &q->head);
}

void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_entry_t *wait)
{
	
	
	wait->flags |= WQ_FLAG_EXCLUSIVE;
	list_add_tail(&wait->entry, &q->head);
}

void remove_wait_queue(wait_queue_head_t *q, wait_queue_entry_t *wait)
{
	
	
	if (wait->entry.prev != LIST_POISON2)
		list_del(&wait->entry);
}

int autoremove_wake_function(wait_queue_entry_t *wait, unsigned mode, int sync,
			     void *key)
{
	
	
	int ret = default_wake_function(wait, mode, sync, key);

	if (ret && (wait->entry.prev != LIST_POISON2))
		list_del_init(&wait->entry);

	return ret;
}

int woken_wake_function(wait_queue_entry_t *wait, unsigned mode, int sync, void *key)
{
	
	wait->flags |= WQ_FLAG_WOKEN;
	return default_wake_function(wait, mode, sync, key);
}


void __init_waitqueue_head(wait_queue_head_t *q, const char *name,
			   struct lock_class_key *k)
{
	
	
	INIT_LIST_HEAD(&q->head);
}

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
		list_add_tail(&wait.entry, &x->wait.task_list);
		do
			timeout = schedule_timeout(timeout);
		while (!x->done && timeout);
		if (wait.entry.prev != LIST_POISON2)
			list_del(&wait.entry);

		if (!x->done)
			return timeout;
	}
	x->done--;
	return timeout ? : 1;
}

// new addition
static int __wake_up_common(struct wait_queue_head *wq_head, unsigned int mode,
			int nr_exclusive, int wake_flags, void *key,
			wait_queue_entry_t *bookmark)
{
	wait_queue_entry_t *curr, *next=lib_malloc(sizeof(wait_queue_entry_t));
	int cnt = 0;

	lockdep_assert_held(&wq_head->lock);

	if (bookmark && (bookmark->flags & WQ_FLAG_BOOKMARK)) {
		curr = list_next_entry(bookmark, entry);

		list_del(&bookmark->entry);
		bookmark->flags = 0;
	} else
		curr = list_first_entry(&wq_head->head, wait_queue_entry_t, entry);

	if (&curr->entry == &wq_head->head)
		return nr_exclusive;

	if(curr == NULL) printk("WC1");
	if(next == NULL) printk("WC2");
	if(&wq_head->head == NULL) printk("WC3");	

	list_for_each_entry_safe_from(curr, next, &wq_head->head, entry) {
		unsigned flags = curr->flags;
		int ret;

		if (flags & WQ_FLAG_BOOKMARK)
			continue;

		ret = curr->func(curr, mode, wake_flags, key);
		if (ret < 0)
			break;
		if (ret && (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
			break;

		if (bookmark && (++cnt > WAITQUEUE_WALK_BREAK_CNT) &&
				(&next->entry != &wq_head->head)) {
			bookmark->flags = WQ_FLAG_BOOKMARK;
			list_add_tail(&bookmark->entry, &next->entry);
			break;
		}
	}

	return nr_exclusive;
}

//  new addition
static void __wake_up_common_lock(struct wait_queue_head *wq_head, unsigned int mode,
			int nr_exclusive, int wake_flags, void *key)
{
	unsigned long flags;
	wait_queue_entry_t bookmark;

	bookmark.flags = 0;
	bookmark.private = NULL;
	bookmark.func = NULL;
	INIT_LIST_HEAD(&bookmark.entry);

	do {
		spin_lock_irqsave(&wq_head->lock, flags);
		nr_exclusive = __wake_up_common(wq_head, mode, nr_exclusive,
						wake_flags, key, &bookmark);
		spin_unlock_irqrestore(&wq_head->lock, flags);
	} while (bookmark.flags & WQ_FLAG_BOOKMARK);
}

//fx evolved
void __wake_up(wait_queue_head_t *wq_head, unsigned int mode,
	       int nr_exclusive, void *key)
{
	__wake_up_common_lock(wq_head, mode, nr_exclusive, 0, key);
}


void __wake_up_sync_key(struct wait_queue_head *q, unsigned int mode, void *key)
{
	
	
	__wake_up(q, mode, 0x10, key);
}

void
prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_entry_t *wait, int state)
{
	
	
	wait->flags |= WQ_FLAG_EXCLUSIVE;
	if (list_empty(&wait->entry))
		list_add_tail(&wait->entry, &q->head);
	set_current_state(state);
}
void prepare_to_wait(wait_queue_head_t *q, wait_queue_entry_t *wait, int state)
{
	
	
	unsigned long flags;

	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	if (list_empty(&wait->entry))
		__add_wait_queue(q, wait);
	set_current_state(state);
	spin_unlock_irqrestore(&q->lock, flags);
}
void finish_wait(wait_queue_head_t *q, wait_queue_entry_t *wait)
{
	
	
	set_current_state(TASK_RUNNING);
	if (!list_empty(&wait->entry))
		list_del_init(&wait->entry);
}


// yaah this one
int default_wake_function(wait_queue_entry_t  *curr, unsigned mode, int wake_flags,
			  void *key)
{
	
	
	struct task_struct *task = (struct task_struct *)curr->private;
	struct SimTask *lib_task = container_of(task, struct SimTask,
						kernel_task);

	return lib_task_wakeup(lib_task);
}


int wake_bit_function(wait_queue_entry_t *wait, unsigned mode, int sync, void *arg)
{
	
	
	struct wait_bit_key *key = arg;
	struct wait_bit_queue_entry *wait_bit
		= container_of(wait, struct wait_bit_queue_entry, wq_entry);

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

#define WAIT_TABLE_BITS 8
#define WAIT_TABLE_SIZE (1 << WAIT_TABLE_BITS)

static wait_queue_head_t bit_wait_table[WAIT_TABLE_SIZE] __cacheline_aligned;

wait_queue_head_t *bit_waitqueue(void *word, int bit)
{
	const int shift = BITS_PER_LONG == 32 ? 5 : 6;
	const struct zone *zone = page_zone(virt_to_page(word));
	unsigned long val = (unsigned long)word << shift | bit;

	return bit_wait_table + hash_long(val, WAIT_TABLE_BITS);
}

void wake_up_bit(void *word, int bit)
{	
	
	
	return;
	__wake_up_bit(bit_waitqueue(word, bit), word, bit);
}

__sched int bit_wait(struct wait_bit_key *word, int mode)
{
	if (signal_pending_state(current->state, current))
		return 1;
	schedule();
	return 0;
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

// yaah this one
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

void yield(void)
{
	
	
	lib_task_yield();
}

signed long schedule_timeout_uninterruptible(signed long timeout)
{
	
	
	return schedule_timeout(timeout);
}

signed long schedule_timeout_interruptible(signed long timeout)
{
	
	
	return schedule_timeout(timeout);
}

void complete_all(struct completion *x)
{
	
	
	x->done += UINT_MAX / 2;
	__wake_up(&x->wait, TASK_NORMAL, 0, 0);
}

void complete(struct completion *x)
{
	
	
	if (!x)
		return;

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

int wait_for_completion_killable(struct completion *x)
{	
	
	
	wait_for_completion_timeout(x, MAX_SCHEDULE_TIMEOUT);
	return 0;
}


// yaah this one
int wake_up_process(struct task_struct *tsk)
{
	
	struct SimTask *lib_task =
		container_of(tsk, struct SimTask, kernel_task);

	return lib_task_wakeup(lib_task);
}

int __cond_resched(void)
{	
	return 0;
}

int idle_cpu(int cpu)
{	
	
	
	return 0;
}

unsigned long long __attribute__((weak)) sched_clock(void)
{
	
	
	return (unsigned long long)(jiffies - INITIAL_JIFFIES)
	       * (NSEC_PER_SEC / HZ);
}


void __sched schedule_preempt_disabled(void)
{
	
	
}

void resched_cpu(int cpu)
{
	
	
	rcu_qs();
}

int _cond_resched(void){
	rcu_qs();
	return 0;
}

void __init_swait_queue_head(struct swait_queue_head *q, const char *name,
			     struct lock_class_key *key)
{
	// We removed comments from first two lines
	raw_spin_lock_init(&q->lock);
	lockdep_set_class_and_name(&q->lock, key, name);
	INIT_LIST_HEAD(&q->task_list);
}

// added by me 
long wait_woken(struct wait_queue_entry *wq_entry, unsigned mode, long timeout)
{
	printk("Added : %s",__func__);	
	timeout = schedule_timeout(timeout);
	__set_current_state(TASK_RUNNING);	

	smp_store_mb(wq_entry->flags, wq_entry->flags & ~WQ_FLAG_WOKEN);

	return timeout;	
}

