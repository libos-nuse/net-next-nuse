#ifndef _ASM_SIM_PAGE_H
#define _ASM_SIM_PAGE_H

typedef struct {} pud_t;

#define THREAD_ORDER    1
#define THREAD_SIZE  (PAGE_SIZE << THREAD_ORDER)

#define WANT_PAGE_VIRTUAL 1

#include <asm-generic/page.h>
#include <asm-generic/getorder.h>

#endif /* _ASM_SIM_PAGE_H */
