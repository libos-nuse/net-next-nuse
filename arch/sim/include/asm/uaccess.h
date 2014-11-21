#ifndef _ASM_SIM_UACCESS_H
#define _ASM_SIM_UACCESS_H

#define KERNEL_DS ((mm_segment_t) {0 })
#define USER_DS ((mm_segment_t) {0 })
#define get_fs() KERNEL_DS
#define get_ds() USER_DS
#define set_fs(x) do {} while ((x.seg) != (x.seg))

#define __access_ok(addr, size) (1)

#include <asm-generic/uaccess.h>

#endif /* _ASM_SIM_UACCESS_H */
