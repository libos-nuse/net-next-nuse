#ifndef _ASM_SIM_ELF_H
#define _ASM_SIM_ELF_H

#if defined(CONFIG_64BIT)
#define ELF_CLASS ELFCLASS64
#else
#define ELF_CLASS ELFCLASS32
#endif

#define task_pt_regs(t) NULL

#endif /* _ASM_SIM_ELF_H */
