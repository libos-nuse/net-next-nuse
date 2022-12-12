#ifndef _ASM_SIM_THREAD_INFO_H
#define _ASM_SIM_THREAD_INFO_H

#define TIF_NEED_RESCHED 1
#define TIF_SIGPENDING 2
#define TIF_MEMDIE 5
#define TIF_NOTIFY_SIGNAL	17
#define TIF_NOTIFY_RESUME	18
#define TIF_SYSCALL_TRACE 8

#define THREAD_SIZE_ORDER	1
struct thread_info {
	__u32 flags;
	int preempt_count;
	struct task_struct *task;
	struct restart_block restart_block;
};

struct thread_info *current_thread_info(void);
struct thread_info *alloc_thread_info(struct task_struct *task);
void free_thread_info(struct thread_info *ti);

#define TS_RESTORE_SIGMASK      0x0008  /* restore signal mask in do_signal() */

#endif /* _ASM_SIM_THREAD_INFO_H */
