#include <asm-generic/barrier.h>

#undef smp_store_release
#define smp_store_release(p, v)						\
	do {								\
		smp_mb();						\
		WRITE_ONCE(*p,v);					\
	} while (0)

#undef smp_load_acquire
#define smp_load_acquire(p)						\
({									\
	typeof(*p) ___p1 = READ_ONCE(*p);				\
	smp_mb();							\
	___p1;								\
})
