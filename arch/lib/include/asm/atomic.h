#ifndef _ASM_SIM_ATOMIC_H
#define _ASM_SIM_ATOMIC_H

#include <linux/types.h>
#include <asm-generic/cmpxchg.h>

#if !defined(CONFIG_64BIT)
typedef struct {
	volatile long long counter;
} atomic64_t;
#endif

#define ATOMIC64_INIT(i) { (i) }

#define atomic64_read(v)        (*(volatile long *)&(v)->counter)
static inline void atomic64_add(long i, atomic64_t *v)
{
	v->counter += i;
}static inline void atomic64_sub(long i, atomic64_t *v)
{
	v->counter -= i;
}
static inline void atomic64_inc(atomic64_t *v)
{
	v->counter++;
}
int atomic64_sub_and_test(long i, atomic64_t *v);
#define atomic64_dec(v)			atomic64_sub(1LL, (v))
int atomic64_dec_and_test(atomic64_t *v);
int atomic64_inc_and_test(atomic64_t *v);
int atomic64_add_negative(long i, atomic64_t *v);
/* long atomic64_add_return(long i, atomic64_t *v); */
static inline long atomic64_add_return(long i, atomic64_t *v)
{
	v->counter += i;
	return v->counter;
}
static inline void atomic64_set(atomic64_t *v, long i)
{
	v->counter = i;
}
long atomic64_sub_return(long i, atomic64_t *v);
#define atomic64_inc_return(v)  (atomic64_add_return(1, (v)))
#define atomic64_dec_return(v)  (atomic64_sub_return(1, (v)))
static inline long atomic64_cmpxchg(atomic64_t *v, long old, long new)
{
	long long val;

	val = v->counter;
	if (val == old)
		v->counter = new;
	return val;
}
long atomic64_xchg(atomic64_t *v, long new);
int atomic64_add_unless(atomic64_t *v, long a, long u);
int atomic64_inc_is_not_zero(atomic64_t *v);
#define atomic64_inc_not_zero(v) 	atomic64_add_unless((v), 1LL, 0LL)

#include <asm-generic/atomic.h>

#endif /* _ASM_SIM_ATOMIC_H */
