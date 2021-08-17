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
}
static inline void atomic64_sub(long i, atomic64_t *v)
{
	v->counter -= i;
}
static inline long atomic64_add_return(long i, atomic64_t *v)
{
	v->counter += i;
	return v->counter;
}
static inline void atomic64_set(atomic64_t *v, long i)
{
	v->counter = i;
}
static void atomic64_and(long long i, atomic64_t *v)
{
	v->counter &= i;
}

static void atomic64_or(long i, atomic64_t *v)
{
	v->counter |= i;
}

static void atomic64_xor(long i, atomic64_t *v)
{
	v->counter ^= i;
}

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

static __always_inline s64
atomic64_fetch_add(s64 i, atomic64_t *v)
{	
	atomic64_add(i,v);	
	return v->counter;
}

static __always_inline s64
atomic64_fetch_sub(s64 i, atomic64_t *v)
{	
	atomic64_sub(i,v);	
	return v->counter;
}

static __always_inline s64
atomic64_fetch_and(s64 i, atomic64_t *v)
{		
	atomic64_and(i,v);
	return  v->counter;
}

static __always_inline s64
atomic64_fetch_or(s64 i, atomic64_t *v)
{		
	atomic64_or(i,v);
	return  v->counter;
}

static __always_inline s64
atomic64_fetch_xor(s64 i, atomic64_t *v)
{		
	atomic64_xor(i,v);
	return  v->counter;
}

static __always_inline s64
atomic64_sub_return(s64 i, atomic64_t *v)
{		
	atomic64_sub(i,v);
	return v->counter;
}

static __always_inline s64
atomic64_xchg(atomic64_t *v, s64 i)
{
	s64 ret;
	ret = v->counter;
	v->counter = i;
	return ret;
}



#include <asm-generic/atomic.h>

#endif /* _ASM_SIM_ATOMIC_H */
