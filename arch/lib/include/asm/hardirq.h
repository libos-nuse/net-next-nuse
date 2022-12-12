#ifndef _ASM_SIM_HARDIRQ_H
#define _ASM_SIM_HARDIRQ_H

extern unsigned int interrupt_pending;

#define local_softirq_pending_ref (interrupt_pending)

#endif /* _ASM_SIM_HARDIRQ_H */
