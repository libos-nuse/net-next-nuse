#ifndef _ASM_SIM_ELF_H
#define _ASM_SIM_ELF_H

#if defined(CONFIG_64BIT)
#define ELF_CLASS ELFCLASS64
#else
#define ELF_CLASS ELFCLASS32
#endif

#include <asm/ptrace.h>
#include <asm/user.h>

typedef struct pt_regs bpf_user_pt_regs_t;

typedef unsigned long elf_greg_t;
#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];
typedef struct user_i387_struct elf_fpregset_t;

#endif /* _ASM_SIM_ELF_H */