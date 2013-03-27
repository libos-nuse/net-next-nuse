#ifndef _ASM_SIM_PROCESSOR_H
#define _ASM_SIM_PROCESSOR_H

struct thread_struct {};

#define cpu_relax() 
#define KSTK_ESP(tsk)	(0)

void *current_text_addr(void);

#define TASK_SIZE ((~(long)0))

#define thread_saved_pc(x) (unsigned long)0

int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);

#endif /* _ASM_SIM_PROCESSOR_H */
