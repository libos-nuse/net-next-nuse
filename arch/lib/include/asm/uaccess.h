#ifndef _ASM_SIM_UACCESS_H
#define _ASM_SIM_UACCESS_H

#define KERNEL_DS ((mm_segment_t) {0 })
#define USER_DS ((mm_segment_t) {0 })
#define get_fs() KERNEL_DS
#define get_ds() USER_DS
#define set_fs(x) do {} while ((x.seg) != (x.seg))

#define access_ok(addr, size) 1

#define __access_ok(addr, size) (1)

void *lib_memcpy(void *dst, const void *src, unsigned long size);

static inline __must_check unsigned long
raw_copy_from_user(void *to, const void __user * from, unsigned long n)
{
	lib_memcpy(to, (const void __force *)from, n);
	return 0;
}

static inline __must_check unsigned long
raw_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	lib_memcpy((void __force *)to, from, n);
	return 0;
}

#include <asm-generic/uaccess.h>

#endif /* _ASM_SIM_UACCESS_H */
