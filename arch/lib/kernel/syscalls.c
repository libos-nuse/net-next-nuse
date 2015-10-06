/*
 * system call table for Linux libos kernel
 * Copyright (c) 2015 Hajime Tazaki
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */


//#include <linux/syscalls.h>
#include <asm-generic/errno.h>
#include <linux/linkage.h>

/* For prototype declartion */
#define __SYSCALL_COMMON(nr, sym, compat) __SYSCALL_64(nr, sym, compat)
#define __SYSCALL_X32(nr, sym, compat) /* Not supported */

#define __SYSCALL_64(nr, sym, compat) extern asmlinkage void sym(void) ;
#include <asm/syscalls_64.h>


/* For system call table generate */
#undef __SYSCALL_64
#define __SYSCALL_64(nr, sym, compat) [ nr ] = sym,

asmlinkage long sys_ni_syscall(void)
{
	return -ENOSYS;
}

typedef void (*sys_call_ptr_t)(void);
asmlinkage const sys_call_ptr_t rump_sys_call_table[__NR_syscall_max+1] = {
	/*
	 * Smells like a compiler bug -- it doesn't work
	 * when the & below is removed.
	 */
	[0 ... __NR_syscall_max] = (sys_call_ptr_t)&sys_ni_syscall,
#include <asm/syscalls_64.h>
};
