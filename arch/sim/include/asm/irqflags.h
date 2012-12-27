#ifndef _ASM_SIM_IRQFLAGS_H
#define _ASM_SIM_IRQFLAGS_H

unsigned long __raw_local_save_flags(void);

#define __raw_local_save_flags __raw_local_save_flags

#include <asm-generic/irqflags.h>

#endif /* _ASM_SIM_IRQFLAGS_H */
