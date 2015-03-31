#ifndef _ASM_SIM_THREAD_INFO_H
#define _ASM_SIM_THREAD_INFO_H

#define TIF_NEED_RESCHED 1
#define TIF_SIGPENDING 2
#define TIF_MEMDIE 5

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
#define HAVE_SET_RESTORE_SIGMASK        1
static inline void set_restore_sigmask(void)
{
}
static inline void clear_restore_sigmask(void)
{
}
static inline bool test_restore_sigmask(void)
{
	return true;
}
static inline bool test_and_clear_restore_sigmask(void)
{
	return true;
}

#endif /* _ASM_SIM_THREAD_INFO_H */
