#include <asm-generic/barrier.h>

#undef smp_store_release
#define smp_store_release(p, v)						\
	do {								\
		smp_mb();						\
		ACCESS_ONCE(*p) = (v);					\
	} while (0)
