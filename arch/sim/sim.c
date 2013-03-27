#include <linux/init.h> // initcall_t
#include <linux/kernel.h> // SYSTEM_BOOTING
#include <linux/sched.h> // struct task_struct
#include <stdio.h>
#include "sim-init.h"
#include "sim.h"

extern void sk_init (void);
extern void skb_init (void);

enum system_states system_state = SYSTEM_BOOTING;

static struct SimImported g_imported;

extern void sim_softirq_wakeup (void);
extern void sim_update_jiffies (void);

#define RETURN_void(rettype, v)			\
  ({						\
    (v);					\
    sim_softirq_wakeup ();			\
  })
#define RETURN_nvoid(rettype, v)		\
  ({						\
    rettype x = (v);				\
    sim_softirq_wakeup ();			\
    x;						\
  })

#define FORWARDER1(name,type,rettype,t0)		\
  extern rettype name (t0);				\
  static rettype name ## _forwarder (t0 v0)		\
  {							\
    sim_update_jiffies ();				\
    return RETURN_##type (rettype, (name (v0)));	\
  }
#define FORWARDER2(name,type,rettype,t0,t1)		\
  extern rettype name (t0,t1);				\
  static rettype name ## _forwarder (t0 v0, t1 v1)	\
  {							\
    sim_update_jiffies ();				\
    return RETURN_##type (rettype, (name (v0, v1)));	\
  }
#define FORWARDER3(name,type,rettype,t0,t1,t2)				\
  extern rettype name (t0,t1,t2);					\
  static rettype name ## _forwarder (t0 v0, t1 v1, t2 v2)		\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (name (v0, v1, v2)));		\
  }
#define FORWARDER4(name,type,rettype,t0,t1,t2,t3)			\
  extern rettype name (t0,t1,t2,t3);					\
  static rettype name ## _forwarder (t0 v0, t1 v1, t2 v2, t3 v3)	\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (name (v0, v1, v2, v3)));		\
  }
#define FORWARDER4(name,type,rettype,t0,t1,t2,t3)			\
  extern rettype name (t0,t1,t2,t3);					\
  static rettype name ## _forwarder (t0 v0, t1 v1, t2 v2, t3 v3)	\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (name (v0, v1, v2, v3)));		\
  }
#define FORWARDER5(name,type,rettype,t0,t1,t2,t3,t4)			\
  extern rettype name (t0,t1,t2,t3,t4);					\
  static rettype name ## _forwarder (t0 v0, t1 v1, t2 v2, t3 v3, t4 v4)	\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (name (v0, v1, v2, v3, v4)));	\
  }

extern struct SimTask *sim_task_create (void *private, unsigned long pid);
extern void sim_task_destroy (struct SimTask *task);
extern void *sim_task_get_private (struct SimTask *task);

FORWARDER2(sim_dev_create, nvoid, struct SimDevice *, void *, enum SimDevFlags);
FORWARDER1(sim_dev_destroy, void, void, struct SimDevice *);
FORWARDER2(sim_dev_set_address, void, void, struct SimDevice *, unsigned char *);
FORWARDER2(sim_dev_set_mtu, void, void, struct SimDevice *, int);
extern void *sim_dev_get_private (struct SimDevice *);
FORWARDER2(sim_dev_create_packet,nvoid, struct SimDevicePacket, struct SimDevice *, int);
FORWARDER2(sim_dev_rx,void, void, struct SimDevice *, struct SimDevicePacket);

FORWARDER4(sim_sock_socket, nvoid, int, int, int, int, struct SimSocket **);
FORWARDER1(sim_sock_close, nvoid, int, struct SimSocket *);
FORWARDER3(sim_sock_recvmsg, nvoid, ssize_t, struct SimSocket *,struct msghdr *, int);
FORWARDER3(sim_sock_sendmsg, nvoid, ssize_t, struct SimSocket *,const struct msghdr *, int);
FORWARDER3(sim_sock_getsockname, nvoid, int, struct SimSocket *,struct sockaddr *, int *);
FORWARDER3(sim_sock_getpeername, nvoid, int, struct SimSocket *,struct sockaddr *, int *);
FORWARDER3(sim_sock_bind, nvoid, int, struct SimSocket *,const struct sockaddr *, int);
FORWARDER4(sim_sock_connect, nvoid, int, struct SimSocket *,const struct sockaddr *, int, int);
FORWARDER2(sim_sock_listen,nvoid, int, struct SimSocket *,int);
FORWARDER2(sim_sock_shutdown,nvoid, int, struct SimSocket *,int);
FORWARDER3(sim_sock_accept,nvoid, int, struct SimSocket *,struct SimSocket **, int);
FORWARDER3(sim_sock_ioctl,nvoid, int, struct SimSocket *, int, char *);
FORWARDER5(sim_sock_setsockopt,nvoid, int, struct SimSocket *, int, int, const void *, int);
FORWARDER5(sim_sock_getsockopt,nvoid, int, struct SimSocket *, int, int, void *, int *);

FORWARDER2(sim_sock_poll, void, void, struct SimSocket *, void *);
FORWARDER1(sim_sock_pollfreewait, void, void, void *);

FORWARDER1(sim_sys_iterate_files,void,void,const struct SimSysIterator *);
FORWARDER4(sim_sys_file_read,nvoid,int,const struct SimSysFile *, char *, int, int);
FORWARDER4(sim_sys_file_write,nvoid,int,const struct SimSysFile *, const char *, int, int);

extern void sim_proc_net_initialize(void);

extern int devices_init (void);
extern void idr_init_cache (void);
extern void rcu_init (void);
extern void rcu_idle_enter (void);
extern void rcu_idle_exit (void);

static struct SimKernel *g_kernel;


static int num_handler = 0;
void *atexit_list[1024];
FILE *stderr = NULL;

void sim_init (struct SimExported *exported, const struct SimImported *imported, struct SimKernel *kernel)
{
  // make sure we can call the callbacks
  g_imported = *imported;
  g_kernel = kernel;
  exported->task_create = sim_task_create;
  exported->task_destroy = sim_task_destroy;
  exported->task_get_private = sim_task_get_private;
  exported->sock_socket = sim_sock_socket_forwarder;
  exported->sock_close = sim_sock_close_forwarder;
  exported->sock_recvmsg = sim_sock_recvmsg_forwarder;
  exported->sock_sendmsg = sim_sock_sendmsg_forwarder;
  exported->sock_getsockname = sim_sock_getsockname_forwarder;
  exported->sock_getpeername = sim_sock_getpeername_forwarder;
  exported->sock_bind = sim_sock_bind_forwarder;
  exported->sock_connect = sim_sock_connect_forwarder;
  exported->sock_listen = sim_sock_listen_forwarder;
  exported->sock_shutdown = sim_sock_shutdown_forwarder;
  exported->sock_accept = sim_sock_accept_forwarder;
  exported->sock_ioctl = sim_sock_ioctl_forwarder;
  exported->sock_setsockopt = sim_sock_setsockopt_forwarder;
  exported->sock_getsockopt = sim_sock_getsockopt_forwarder;

  exported->sock_poll = sim_sock_poll_forwarder;
  exported->sock_pollfreewait = sim_sock_pollfreewait_forwarder;

  exported->dev_create = sim_dev_create_forwarder;
  exported->dev_destroy = sim_dev_destroy_forwarder;
  exported->dev_get_private = sim_dev_get_private;
  exported->dev_set_address = sim_dev_set_address_forwarder;
  exported->dev_set_mtu = sim_dev_set_mtu_forwarder;
  exported->dev_create_packet = sim_dev_create_packet_forwarder;
  exported->dev_rx = sim_dev_rx_forwarder;

  exported->sys_iterate_files = sim_sys_iterate_files_forwarder;
  exported->sys_file_write = sim_sys_file_write_forwarder;
  exported->sys_file_read = sim_sys_file_read_forwarder;

  rcu_init ();

  // in drivers/base/core.c (called normally by drivers/base/init.c)
  devices_init();
  // in lib/idr.c (called normally by init/main.c)
  idr_init_cache();

  sim_proc_net_initialize();

  // and, then, call the normal initcalls
  initcall_t *call;
  extern initcall_t __initcall_start[], __initcall_end[];

  call = __initcall_start;
  do {
    (*call)();
    call++;
  } while (call < __initcall_end);

  // finally, put the system in RUNNING state.
  system_state = SYSTEM_RUNNING;

  /* XXX handle atexit registration for gcov */
  int i;
  for (i = 0; i < 1024; i++)
    {
      if (atexit_list [i])
        g_imported.atexit (g_kernel, (void (*)(void))atexit_list[i]);
    }

}


int sim_vprintf (const char *str, va_list args)
{
  return g_imported.vprintf (g_kernel, str, args);
}
void *sim_malloc (unsigned long size)
{
  return g_imported.malloc (g_kernel, size);
}
void sim_free (void *buffer)
{
  return g_imported.free (g_kernel, buffer);
}
void *sim_memcpy (void *dst, const void *src, unsigned long size)
{
  return g_imported.memcpy (g_kernel, dst, src, size);
}
void *sim_memset (void *dst, char value, unsigned long size)
{
  return g_imported.memset (g_kernel, dst, value, size);
}
int atexit (void (*function)(void))
{
  if (g_imported.atexit == 0)
    {
      atexit_list[num_handler++] = function;
      return 0;
    }
  else
    {
      return g_imported.atexit (g_kernel, function);
    }
}
int access (const char *pathname, int mode)
{
  return g_imported.access (g_kernel, pathname, mode);
}
char *getenv (const char *name)
{
  return g_imported.getenv (g_kernel, name);
}
pid_t getpid(void)
{
  return (pid_t)0;
}
int mkdir(const char *pathname, mode_t mode)
{
  return g_imported.mkdir (g_kernel, pathname, mode);
}
int open(const char *pathname, int flags)
{
  return g_imported.open (g_kernel, pathname, flags);
}
int fcntl(int fd, int cmd, ... /* arg */ )
{
  return 0;
}
int __fxstat (int ver, int fd, void *buf)
{
  return g_imported.__fxstat (g_kernel, ver, fd, buf);
}
int fseek(FILE *stream, long offset, int whence)
{
  return g_imported.fseek (g_kernel, stream, offset, whence);
}
long ftell(FILE *stream)
{
  return g_imported.ftell (g_kernel, stream);
}
void setbuf(FILE *stream, char *buf)
{
  return g_imported.setbuf (g_kernel, stream, buf);
}
FILE *fdopen(int fd, const char *mode)
{
  return g_imported.fdopen (g_kernel, fd, mode);
}
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return g_imported.fread (g_kernel, ptr, size, nmemb, stream);
}
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return g_imported.fwrite (g_kernel, ptr, size, nmemb, stream);
}
int fclose(FILE *fp)
{
  return g_imported.fclose (g_kernel, fp);
}
unsigned long sim_random (void)
{
  return g_imported.random (g_kernel);
}
void *sim_event_schedule_ns (__u64 ns, void (*fn) (void *context), void *context)
{
  return g_imported.event_schedule_ns (g_kernel, ns, fn, context, sim_update_jiffies);
}
void sim_event_cancel (void *event)
{
  return g_imported.event_cancel (g_kernel, event);
}
__u64 sim_current_ns (void)
{
  return g_imported.current_ns (g_kernel);
}
struct SimTaskTrampolineContext
{
  void (*callback) (void *);
  void *context;
};
static void sim_task_start_trampoline (void *context)
{
  // we use this trampoline solely for the purpose of executing sim_update_jiffies
  // prior to calling the callback.
  struct SimTaskTrampolineContext *ctx = context;
  void (*callback) (void *) = ctx->callback;
  void *callback_context = ctx->context;
  sim_free (ctx);
  sim_update_jiffies ();
  callback (callback_context);
}
struct SimTask *sim_task_start (void (*callback) (void *), void *context)
{
  struct SimTaskTrampolineContext *ctx = sim_malloc (sizeof (struct SimTaskTrampolineContext));
  if (!ctx)
    {
      return NULL;
    }
  ctx->callback = callback;
  ctx->context = context;
  return g_imported.task_start (g_kernel, &sim_task_start_trampoline, ctx);
}
extern void rcu_sched_qs (int);
void sim_task_wait (void)
{
  rcu_sched_qs (0);
  g_imported.task_wait (g_kernel);
  sim_update_jiffies ();
}
struct SimTask *sim_task_current (void)
{
  return g_imported.task_current (g_kernel);
}
int sim_task_wakeup (struct SimTask *task)
{
  return g_imported.task_wakeup (g_kernel, task);
}
void sim_task_yield (void)
{
  rcu_idle_enter ();
  g_imported.task_yield (g_kernel);
  rcu_idle_exit ();
  sim_update_jiffies ();
}

void sim_dev_xmit (struct SimDevice *dev, unsigned char *data, int len)
{
  return g_imported.dev_xmit (g_kernel, dev, data, len);
}

void sim_signal_raised (struct SimTask *task, int sig)
{
  g_imported.signal_raised (g_kernel, task , sig);
}

void sim_poll_event (int flag, void *context)
{
  g_imported.poll_event (flag, context);
}
