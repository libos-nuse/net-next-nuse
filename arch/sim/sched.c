#include <linux/wait.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>
#include <net/net_namespace.h>
#include "sim.h"
#include "sim-assert.h"

/**
called by wait_event macro:
 - prepare_to_wait
 - schedule
 - finish_wait
*/

struct SimTask
{
  struct task_struct kernel_task;
  void *private;
};

struct SimTask *sim_task_create (void *private, unsigned long pid)
{
  struct SimTask *task = sim_malloc (sizeof (struct SimTask));
  if (!task) return NULL;
  memset (task, 0, sizeof (struct SimTask));
  struct cred *cred = sim_malloc (sizeof (struct cred));
  if (!cred) return NULL;
  // XXX: we could optimize away this allocation by sharing it for all tasks
  struct nsproxy *ns = sim_malloc (sizeof (struct nsproxy));
  if (!ns) return NULL;
  struct user_struct *user = sim_malloc (sizeof (struct user_struct));
  if (!user) return NULL;
  struct thread_info *info = alloc_thread_info (&task->kernel_task);
  if (!info) return NULL;
  struct pid *kpid = sim_malloc (sizeof (struct pid));
  if (!kpid) return NULL;
  kpid->numbers[0].nr = pid;
  cred->fsuid = make_kuid (current_user_ns (), 0);
  cred->fsgid = make_kgid (current_user_ns (), 0);
  cred->user = user;
  atomic_set (&cred->usage, 1);
  info->task = &task->kernel_task;
  info->preempt_count = 0;
  info->flags = 0;
  atomic_set (&ns->count, 1);
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
  task->kernel_task.group_leader = &task->kernel_task; // this is a hack.
  task->private = private;
  return task;
}
void sim_task_destroy (struct SimTask *task)
{
  sim_free ((void *)task->kernel_task.nsproxy);
  sim_free ((void *)task->kernel_task.cred);
  sim_free ((void *)task->kernel_task.cred->user);
  free_thread_info (task->kernel_task.stack);
  sim_free (task);
}
void *sim_task_get_private (struct SimTask *task)
{
  return task->private;
}

int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
  struct SimTask *task = sim_task_start ((void (*)(void*))fn, arg);  
  return task->kernel_task.pid;
}

struct task_struct *get_current(void)
{
  struct SimTask *sim_task = sim_task_current ();
  return &sim_task->kernel_task;
}

struct thread_info *current_thread_info(void)
{
  return task_thread_info (get_current ());
}
struct thread_info *alloc_thread_info (struct task_struct *task)
{
  return sim_malloc (sizeof (struct thread_info));
}
void free_thread_info (struct thread_info *ti)
{
  sim_free (ti);
}


void __put_task_struct(struct task_struct *t)
{
  sim_free (t);
}

void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
  wait->flags &= ~WQ_FLAG_EXCLUSIVE;
  list_add (&wait->task_list, &q->task_list);
}
void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait)
{
  wait->flags |= WQ_FLAG_EXCLUSIVE;
  list_add(&wait->task_list, &q->task_list);
}
void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
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
    {
      list_del_init(&wait->task_list);
    }
}
int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
  int ret = default_wake_function(wait, mode, sync, key);
  if (ret)
    list_del_init(&wait->task_list);
  return ret;
}

void __init_waitqueue_head(wait_queue_head_t *q, const char *name, struct lock_class_key *k)
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
unsigned long wait_for_completion_timeout(struct completion *x, unsigned long timeout)
{
  if (!x->done) 
    {
      DECLARE_WAITQUEUE(wait, current);
      set_current_state(TASK_UNINTERRUPTIBLE);
      wait.flags |= WQ_FLAG_EXCLUSIVE;
      list_add_tail(&wait.task_list, &x->wait.task_list);
      do {
	timeout = schedule_timeout (timeout);
      } while (!x->done && timeout);
      list_del (&wait.task_list);
      if (!x->done)
	return timeout;
    }
  x->done--;
  return timeout ?: 1;
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
  __wake_up (q, mode, nr_exclusive, key);
}
int default_wake_function(wait_queue_t *curr, unsigned mode, int wake_flags,
                          void *key)
{
  struct task_struct *task = (struct task_struct *)curr->private;
  struct SimTask *sim_task = (struct SimTask *)task;
  return sim_task_wakeup (sim_task);
}


void schedule (void)
{
  sim_task_wait ();
}

static void trampoline (void *context)
{
  struct SimTask *task = context;
  sim_task_wakeup (task);
}

signed long schedule_timeout(signed long timeout)
{
  if (timeout == MAX_SCHEDULE_TIMEOUT)
    {
      sim_task_wait ();
      return MAX_SCHEDULE_TIMEOUT;
    }
  sim_assert (timeout >= 0);
  u64 ns = ((__u64) timeout) * (1000000000 / HZ);
  struct SimTask *self = sim_task_current ();
  sim_event_schedule_ns (ns, &trampoline, self);
  sim_task_wait ();
  // we know that we are always perfectly on time.
  return 0;
}

signed long schedule_timeout_uninterruptible(signed long timeout)
{
  return schedule_timeout (timeout);
}
signed long schedule_timeout_interruptible(signed long timeout)
{
  return schedule_timeout (timeout);
}

void yield (void)
{
  sim_task_yield ();
}

void complete_all(struct completion *x)
{
  x->done += UINT_MAX/2;
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
  return sim_task_wakeup((struct SimTask *)tsk);
}
int _cond_resched(void)
{
  // we never schedule to decrease latency.
  return 0;
}
int idle_cpu (int cpu)
{
  // we are never idle: we call this from rcutiny.c and the answer
  // does not matter, really.
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
  return;
}
