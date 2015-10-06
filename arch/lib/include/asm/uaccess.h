#ifndef _ASM_SIM_UACCESS_H
#define _ASM_SIM_UACCESS_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/errno.h>
#include "sim.h"

/* how to include rump headers into arch/lib ? */
#ifdef FIXME
#include <rump/rumpuser.h>
#else
int rumpuser_sp_copyin(void *, const void *, void *, size_t);
int rumpuser_sp_copyout(void *, const void *, void *, size_t);
void *rump_is_remote_client(void);
#endif


#define __access_ok(addr, size) (1)

/* handle rump remote client */
static inline __must_check long __copy_from_user(void *to,
		const void __user * from, unsigned long n)
{
	int error = 0;

	if (unlikely(from == NULL && n)) {
		return EFAULT;
	}

	/* FIXME: how to handle the DCE's case ? */
	if (!rump_is_remote_client()) {
		lib_memcpy(to, from, n);
	} else if (n) {
		error = rumpuser_sp_copyin(rump_is_remote_client(), from, to, n);
	}

	return error;
}
#define __copy_from_user(to,from,n) __copy_from_user(to,from,n)

static inline __must_check long __copy_to_user(void __user *to,
		const void *from, unsigned long n)
{
	int error = 0;

	if (unlikely(to == NULL && n)) {
		return EFAULT;
	}

	/* FIXME: how to handle the DCE's case ? */
	if (!rump_is_remote_client()) {
		lib_memcpy(to, from, n);
	}
	else if (n) {
		error = rumpuser_sp_copyout(rump_is_remote_client(), from, to, n);
	}

	return error;
}
#define __copy_to_user(to,from,n) __copy_to_user(to,from,n)

#include <asm-generic/uaccess.h>

#endif /* _ASM_SIM_UACCESS_H */
