/*
 * library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/init.h>         /* initcall_t */
#include <linux/kernel.h>       /* SYSTEM_BOOTING */
#include <linux/sched.h>        /* struct task_struct */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/fs_struct.h>
#include <linux/slab.h>
#include <drivers/base/base.h>
#include <linux/idr.h>
#include <linux/rcupdate.h>
#include <linux/init_task.h>
#include <linux/fdtable.h>
#include "sim-init.h"
#include "sim.h"

enum system_states system_state = SYSTEM_BOOTING;
/* glues */
struct task_struct init_task;

struct SimImported g_imported;


#define RETURN_void(rettype, v)				     \
	({						     \
		(v);					     \
		lib_softirq_wakeup();			     \
	})

#define RETURN_nvoid(rettype, v)			     \
	({						     \
		rettype x = (v);			     \
		lib_softirq_wakeup();			     \
		x;					     \
	})

#define FORWARDER1(name, type, rettype, t0)			    \
	extern rettype name(t0);				    \
	static rettype name ## _forwarder(t0 v0)		    \
	{							    \
		lib_update_jiffies();				    \
		return RETURN_ ## type(rettype, (name(v0)));        \
	}

#define FORWARDER2(name, type, rettype, t0, t1)				\
	extern rettype name(t0, t1);					\
	static rettype name ## _forwarder(t0 v0, t1 v1)			\
	{								\
		lib_update_jiffies();					\
		return RETURN_ ## type(rettype, (name(v0, v1)));	\
	}
#define FORWARDER3(name, type, rettype, t0, t1, t2)			\
	extern rettype name(t0, t1, t2);				\
	static rettype name ## _forwarder(t0 v0, t1 v1, t2 v2)		\
	{								\
		lib_update_jiffies();					\
		return RETURN_ ## type(rettype, (name(v0, v1, v2)));	\
	}
#define FORWARDER4(name, type, rettype, t0, t1, t2, t3)			\
	extern rettype name(t0, t1, t2, t3);				\
	static rettype name ## _forwarder(t0 v0, t1 v1, t2 v2, t3 v3)	\
	{								\
		lib_update_jiffies();					\
		return RETURN_ ## type(rettype, (name(v0, v1, v2, v3))); \
	}
#define FORWARDER4(name, type, rettype, t0, t1, t2, t3)			\
	extern rettype name(t0, t1, t2, t3);				\
	static rettype name ## _forwarder(t0 v0, t1 v1, t2 v2, t3 v3)	\
	{								\
		lib_update_jiffies();					\
		return RETURN_ ## type(rettype, (name(v0, v1, v2, v3))); \
	}
#define FORWARDER5(name, type, rettype, t0, t1, t2, t3, t4)		\
	extern rettype name(t0, t1, t2, t3, t4);			\
	static rettype name ## _forwarder(t0 v0, t1 v1, t2 v2, t3 v3, t4 v4) \
	{								\
		lib_update_jiffies();					\
		return RETURN_ ## type(rettype, (name(v0, v1, v2, v3, v4))); \
	}

FORWARDER3(lib_dev_create, nvoid, struct SimDevice *, const char *, void *,
	   enum SimDevFlags);
FORWARDER1(lib_dev_destroy, void, void, struct SimDevice *);
FORWARDER2(lib_dev_set_address, void, void, struct SimDevice *,
	   unsigned char *);
FORWARDER2(lib_dev_set_mtu, void, void, struct SimDevice *, int);
FORWARDER2(lib_dev_create_packet, nvoid, struct SimDevicePacket,
	   struct SimDevice *, int);
FORWARDER2(lib_dev_rx, void, void, struct SimDevice *, struct SimDevicePacket);

FORWARDER4(lib_sock_socket, nvoid, int, int, int, int, struct SimSocket **);
FORWARDER1(lib_sock_close, nvoid, int, struct SimSocket *);
FORWARDER3(lib_sock_recvmsg, nvoid, ssize_t, struct SimSocket *,
	   struct msghdr *, int);
FORWARDER3(lib_sock_sendmsg, nvoid, ssize_t, struct SimSocket *,
	   const struct msghdr *, int);
FORWARDER3(lib_sock_getsockname, nvoid, int, struct SimSocket *,
	   struct sockaddr *, int *);
FORWARDER3(lib_sock_getpeername, nvoid, int, struct SimSocket *,
	   struct sockaddr *, int *);
FORWARDER3(lib_sock_bind, nvoid, int, struct SimSocket *,
	   const struct sockaddr *, int);
FORWARDER4(lib_sock_connect, nvoid, int, struct SimSocket *,
	   const struct sockaddr *, int, int);
FORWARDER2(lib_sock_listen, nvoid, int, struct SimSocket *, int);
FORWARDER2(lib_sock_shutdown, nvoid, int, struct SimSocket *, int);
FORWARDER3(lib_sock_accept, nvoid, int, struct SimSocket *,
	   struct SimSocket **, int);
FORWARDER3(lib_sock_ioctl, nvoid, int, struct SimSocket *, int, char *);
FORWARDER5(lib_sock_setsockopt, nvoid, int, struct SimSocket *, int, int,
	   const void *, int);
FORWARDER5(lib_sock_getsockopt, nvoid, int, struct SimSocket *, int, int,
	   void *, int *);

FORWARDER2(lib_sock_poll, void, void, struct SimSocket *, void *);
FORWARDER1(lib_sock_pollfreewait, void, void, void *);

FORWARDER1(lib_sys_iterate_files, void, void, const struct SimSysIterator *);
FORWARDER4(lib_sys_file_read, nvoid, int, const struct SimSysFile *, char *,
	   int, int);
FORWARDER4(lib_sys_file_write, nvoid, int, const struct SimSysFile *,
	   const char *, int, int);
FORWARDER2(lkl_sysctl,nvoid,int,const char *, const char *);
FORWARDER3(lkl_sysctl_get,nvoid,int,const char *, char *, int );

struct SimKernel *g_kernel;

struct kmem_cache * fs_cachep;
extern struct kmem_cache *names_cachep,  *files_cachep;

void lib_init(struct SimExported *exported, const struct SimImported *imported,
	      struct SimKernel *kernel)
{
	/* make sure we can call the callbacks */
	g_imported = *imported;
	g_kernel = kernel;
	exported->task_create = lib_task_create;
	exported->task_destroy = lib_task_destroy;
	exported->task_get_private = lib_task_get_private;
	exported->sock_socket = lib_sock_socket_forwarder;
	exported->sock_close = lib_sock_close_forwarder;
	exported->sock_recvmsg = lib_sock_recvmsg_forwarder;
	exported->sock_sendmsg = lib_sock_sendmsg_forwarder;
	exported->sock_getsockname = lib_sock_getsockname_forwarder;
	exported->sock_getpeername = lib_sock_getpeername_forwarder;
	exported->sock_bind = lib_sock_bind_forwarder;
	exported->sock_connect = lib_sock_connect_forwarder;
	exported->sock_listen = lib_sock_listen_forwarder;
	exported->sock_shutdown = lib_sock_shutdown_forwarder;
	exported->sock_accept = lib_sock_accept_forwarder;
	exported->sock_ioctl = lib_sock_ioctl_forwarder;
	exported->sock_setsockopt = lib_sock_setsockopt_forwarder;
	exported->sock_getsockopt = lib_sock_getsockopt_forwarder;

	exported->sock_poll = lib_sock_poll_forwarder;
	exported->sock_pollfreewait = lib_sock_pollfreewait_forwarder;

	exported->dev_create = lib_dev_create_forwarder;
	exported->dev_destroy = lib_dev_destroy_forwarder;
	exported->dev_get_private = lib_dev_get_private;
	exported->dev_set_address = lib_dev_set_address_forwarder;
	exported->dev_set_mtu = lib_dev_set_mtu_forwarder;
	exported->dev_create_packet = lib_dev_create_packet_forwarder;
	exported->dev_rx = lib_dev_rx_forwarder;

	exported->sys_iterate_files = lib_sys_iterate_files_forwarder;
	exported->sys_file_write = lib_sys_file_write_forwarder;
	exported->sys_file_read = lib_sys_file_read_forwarder;

	exported->lkl_sysctl = lkl_sysctl_forwarder;
	exported->lkl_sysctl_get = lkl_sysctl_get_forwarder;

	pr_notice("%s", linux_banner);

	files_cachep = kmem_cache_create("files_cache",
			sizeof(struct files_struct), 0,
			SLAB_HWCACHE_ALIGN|SLAB_PANIC|SLAB_ACCOUNT,
			NULL);

	fs_cachep = kmem_cache_create("fs_cache",
			sizeof(struct fs_struct), 0,
			SLAB_HWCACHE_ALIGN|SLAB_PANIC|SLAB_ACCOUNT,
			NULL);

	early_security_init();

	vfs_caches_init_early();

	rcu_init();

	/* in drivers/base/core.c (called normally by drivers/base/init.c) */	
	devices_init();	
	buses_init();	

	radix_tree_init();	

	timekeeping_init();	

	cred_init();
	/* in lib/idr.c (called normally by init/main.c) */				
	
	uts_ns_init();

	security_init();	

	//proc_caches_init();
	vfs_caches_init();

	//proc_init_kmemcache();	
	//lib_proc_net_initialize();	
	seq_file_init();

	proc_root_init();

	/* and, then, call the normal initcalls */
	
	initcall_entry_t *call;
	extern initcall_entry_t __initcall_start[], __initcall_end[];

	call = __initcall_start;
	do {		
		initcall_from_entry(call)();				
		call++;		
	} while (call < __initcall_end);

	/* finally, put the system in RUNNING state. */
	system_state = SYSTEM_RUNNING;
}

int lib_vprintf(const char *str, va_list args)
{
	return g_imported.vprintf(g_kernel, str, args);
}
void *lib_malloc(unsigned long size)
{
	return g_imported.malloc(g_kernel, size);
}
void lib_free(void *buffer)
{
	return g_imported.free(g_kernel, buffer);
}
void *lib_memcpy(void *dst, const void *src, unsigned long size)
{
	return g_imported.memcpy(g_kernel, dst, src, size);
}
void *lib_memset(void *dst, char value, unsigned long size)
{
	return g_imported.memset(g_kernel, dst, value, size);
}
unsigned long lib_random(void)
{
	return g_imported.random(g_kernel);
}
void *lib_event_schedule_ns(__u64 ns, void (*fn) (void *context), void *context)
{
	return g_imported.event_schedule_ns(g_kernel, ns, fn, context,
					    lib_update_jiffies);
}
void lib_event_cancel(void *event)
{
	return g_imported.event_cancel(g_kernel, event);
}
__u64 lib_current_ns(void)
{
	return g_imported.current_ns(g_kernel);
}
struct SimTaskTrampolineContext {
	void (*callback)(void *);
	void *context;
};
static void lib_task_start_trampoline(void *context)
{
	/* we use this trampoline solely for the purpose of executing
	   lib_update_jiffies prior to calling the callback. */
	struct SimTaskTrampolineContext *ctx = context;
	void (*callback)(void *) = ctx->callback;
	void *callback_context = ctx->context;

	lib_free(ctx);
	lib_update_jiffies();
	callback(callback_context);
}
struct SimTask *lib_task_start(void (*callback) (void *), void *context)
{
	struct SimTaskTrampolineContext *ctx =
		lib_malloc(sizeof(struct SimTaskTrampolineContext));

	if (!ctx)
		return NULL;
	ctx->callback = callback;
	ctx->context = context;
	return g_imported.task_start(g_kernel, &lib_task_start_trampoline, ctx);
}
void lib_task_wait(void)
{
	rcu_qs();
	g_imported.task_wait(g_kernel);
	lib_update_jiffies();
}
struct SimTask *lib_task_current(void)
{
	return g_imported.task_current(g_kernel);
}
int lib_task_wakeup(struct SimTask *task)
{
	return g_imported.task_wakeup(g_kernel, task);
}
void lib_task_yield(void)
{
	rcu_idle_enter();
	g_imported.task_yield(g_kernel);
	rcu_idle_exit();
	lib_update_jiffies();
}

void lib_dev_xmit(struct SimDevice *dev, unsigned char *data, int len)
{
	return g_imported.dev_xmit(g_kernel, dev, data, len);
}

void lib_signal_raised(struct SimTask *task, int sig)
{
	g_imported.signal_raised(g_kernel, task, sig);
}

void lib_poll_event(int flag, void *context)
{
	g_imported.poll_event(flag, context);
}
