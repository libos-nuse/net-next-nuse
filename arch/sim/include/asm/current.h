#ifndef _ASM_SIM_CURRENT_H
#define _ASM_SIM_CURRENT_H

struct task_struct *get_current(void);
#define current get_current()

#endif /* _ASM_SIM_CURRENT_H */
