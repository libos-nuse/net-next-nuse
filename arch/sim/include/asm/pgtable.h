#ifndef _ASM_SIM_PGTABLE_H
#define _ASM_SIM_PGTABLE_H

#define PAGE_KERNEL ((pgprot_t){0})

#define arch_start_context_switch(prev) do {} while (0)

#define kern_addr_valid(addr)(1)

#endif /* _ASM_SIM_PGTABLE_H */
