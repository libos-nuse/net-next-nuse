#ifndef _ASM_SIM_PROCESSOR_H
#define _ASM_SIM_PROCESSOR_H

struct thread_struct {};

#define cpu_relax()
#define cpu_relax_lowlatency() cpu_relax()
#define KSTK_ESP(tsk)	(0)

# define current_text_addr() ({ __label__ _l; _l: &&_l; })

#define TASK_SIZE ((~(long)0))

#define thread_saved_pc(x) (unsigned long)0
#define task_pt_regs(t) NULL

int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);

#endif /* _ASM_SIM_PROCESSOR_H */
