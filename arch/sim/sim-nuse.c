#include <linux/init.h> // initcall_t
#include <linux/list.h> // linked-list
#include <linux/kernel.h> // SYSTEM_BOOTING
#include <linux/sched.h> // struct task_struct
#include <net/net_namespace.h> 
#define _GNU_SOURCE /* Get RTLD_NEXT */
#include <dlfcn.h>
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include "sim-init.h"
#include "sim-assert.h"
#include "sim-nuse-vif.h"
#include "sim-nuse.h"
#include "sim.h"

int kptr_restrict __read_mostly;
extern struct socket *g_fd_table[1024];

struct SimTask;
enum system_states system_state = SYSTEM_BOOTING;

extern void sim_softirq_wakeup (void);
extern void sim_update_jiffies (void);
extern struct SimTask *sim_task_create (void *private, unsigned long pid);
extern void *sim_dev_get_private (struct SimDevice *);
extern struct SimDevice *sim_dev_create (void *priv, enum SimDevFlags flags);
extern void sim_dev_set_address (struct SimDevice *dev, unsigned char buffer[6]);


/* stdlib.h */
extern void *malloc(size_t size);
extern void free(void *ptr);
extern long int random(void);

extern pid_t getpid(void);
extern int nanosleep(const struct timespec *req, struct timespec *rem);
extern int clock_gettime(clockid_t clk_id, struct timespec *tp);

/* ipaddress config */
extern int devinet_ioctl(struct net *net, unsigned int cmd, void __user *arg);
extern void ether_setup(struct net_device *dev);
typedef uint32_t in_addr_t;
extern in_addr_t inet_addr(const char *cp);


int sim_vprintf (const char *str, va_list args)
{
  return vprintf (str, args);
}
void *sim_malloc (unsigned long size)
{
  return malloc (size);
}
void sim_free (void *buffer)
{
  return free (buffer);
}

void *sim_memcpy (void *dst, const void *src, unsigned long size)
{
  return memcpy (dst, src, size);
}
void *sim_memset (void *dst, char value, unsigned long size)
{
  return memset (dst, value, size);
}
__u64 sim_current_ns (void)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000000000 + tp.tv_nsec;
}
unsigned long sim_random (void)
{
  return random ();
}

static struct SimTask *g_sim_main_ctx = NULL;
struct list_head g_task_lists = LIST_HEAD_INIT (g_task_lists);

struct SimTask *sim_task_current (void)
{
  struct SimTask *task;

  rcu_read_lock();
  list_for_each_entry_rcu(task, &g_task_lists, head) 
    {
      void *fiber = task->private;
      if (fiber && sim_fiber_isself (fiber))
        {
          rcu_read_unlock();
          return task;
        }
    }
  rcu_read_unlock();

  return g_sim_main_ctx;
}

struct SimNuseTaskTrampolineContext
{
  void (*callback) (void *);
  void *context;
  struct SimTask *task;
};

void
sim_nuse_task_add (void *fiber)
{
  struct SimTask *task = sim_task_create (fiber, getpid ());
  list_add_tail_rcu (&task->head, &g_task_lists);
  return;
}

static void *sim_nuse_task_start_trampoline (void *context)
{
  // we use this trampoline solely for the purpose of executing sim_update_jiffies
  // prior to calling the callback.
  struct SimNuseTaskTrampolineContext *ctx = context;
  int found = 0;
  struct SimTask *task;

  list_for_each_entry_rcu(task, &g_task_lists, head) 
    {
      if (task->private == ctx->task->private)
        {
          found = 1;
          break;
        }
    }
  if (!found)
    {
      return NULL;
    }

  if (sim_fiber_is_stopped (ctx->task->private))
    {
      sim_free (ctx);
      sim_update_jiffies ();
      printf ("canceled\n");
      return NULL;
    }

  void (*callback) (void *) = ctx->callback;
  void *callback_context = ctx->context;
  task = ctx->task;
  sim_free (ctx);
  sim_update_jiffies ();

  callback (callback_context);

  //  sim_fiber_free (task->private);
  list_del_rcu (&task->head);

  return ctx;
}

struct SimTask *sim_task_start (void (*callback) (void *), void *context)
{
  struct SimNuseTaskTrampolineContext *ctx = sim_malloc (sizeof (struct SimNuseTaskTrampolineContext));
  if (!ctx)
    {
      return NULL;
    }
  ctx->callback = callback;
  ctx->context = context;

  void *fiber = sim_fiber_new (&sim_nuse_task_start_trampoline, ctx, 1 << 16, "task");
  struct SimTask *task = NULL;
  task = sim_task_create (fiber, getpid ());
  ctx->task = task;

  if (!sim_fiber_is_stopped (task->private))
    {
      list_add_tail_rcu (&task->head, &g_task_lists);
    }

  sim_fiber_start (fiber);
  return task;
}

void *sim_event_schedule_ns (__u64 ns, void (*fn) (void *context), void *context)
{
  struct SimNuseTaskTrampolineContext *ctx = sim_malloc (sizeof (struct SimNuseTaskTrampolineContext));
  if (!ctx)
    {
      return NULL;
    }
  ctx->callback = fn;
  ctx->context = context;

  /* without fiber_start (pthread) */
  void *fiber = sim_fiber_new_from_caller (1 << 16, "task_sched");
  struct SimTask *task = NULL;
  task = sim_task_create (fiber, getpid ());
  ctx->task = task;

  list_add_tail_rcu (&task->head, &g_task_lists);

  sim_add_timer (ns, sim_nuse_task_start_trampoline, ctx, fiber);

  return task;
}

void sim_event_cancel (void *event)
{
  struct SimTask *task = event;
  sim_fiber_stop (task->private);
  //  sim_fiber_free (task->private);
  list_del_rcu (&task->head);
  return;
}

void sim_task_wait (void)
{
  /* FIXME */
  rcu_sched_qs (0);
  struct SimTask *task = sim_task_current ();
  sim_assert (task != NULL);
  sim_fiber_wait (task->private);
  sim_update_jiffies ();
}

int sim_task_wakeup (struct SimTask *task)
{
  return sim_fiber_wakeup (task->private);
}


extern void sim_proc_net_initialize(void);

extern int devices_init (void);
extern void idr_init_cache (void);
extern void rcu_init (void);
extern void rcu_idle_enter (void);
extern void rcu_idle_exit (void);


void *
sim_netdev_rx_trampoline (void *context)
{
  struct SimDevice *dev = context;
  struct nuse_vif *vif = sim_dev_get_private (dev);
  nuse_vif_read (vif, dev);
  /* should not reach */
  return dev;
}

void
sim_dev_xmit (struct SimDevice *dev, unsigned char *data, int len)
{
  struct nuse_vif *vif = sim_dev_get_private (dev);
  nuse_vif_write (vif, dev, data, len);
  sim_softirq_wakeup ();
  return;
}

void sim_signal_raised (struct SimTask *task, int sig)
{
  static int logged = 0;
  if (!logged)
    {
      sim_printf ("%s: Not implemented yet\n", __FUNCTION__);
      logged = 1;
    }
  return;
}

void
sim_netdev_create (void)
{
  int err;

  /* FIXME: shoudl be configurable */
  //  struct nuse_vif *vif = nuse_vif_create (NUSE_VIF_NETMAP, "xge0");
  //struct nuse_vif *vif = nuse_vif_create (NUSE_VIF_RAWSOCK, "xge0");
  struct nuse_vif *vif = nuse_vif_create (NUSE_VIF_RAWSOCK, "ens33");
  if (!vif)
    {
      sim_printf ("vif create error\n");
      sim_assert (0);
      return;
    }

  /* FIXME: temporal impl (sim0). */
  struct SimDevice *dev = sim_dev_create (vif, 0);
  unsigned char mac[6] = {0,0,0,0,0,0x01};
  sim_dev_set_address (dev, mac);
  ether_setup ((struct net_device *)dev);

  struct ifreq ifr;
  struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr ("192.168.209.39");
  //  sin->sin_addr.s_addr = inet_addr ("130.69.250.39");
  strncpy (ifr.ifr_name, "sim0", IFNAMSIZ-1);
  err = devinet_ioctl (&init_net, SIOCSIFADDR, &ifr);
  if (err)
    {
      sim_printf ("err devinet_ioctl %d\n", err);
    }

  /* IFF_UP */
  memset (&ifr, 0, sizeof (struct ifreq));
  ifr.ifr_flags = IFF_UP;
  strncpy (ifr.ifr_name, "sim0", IFNAMSIZ-1);
  err = devinet_ioctl (&init_net, SIOCSIFFLAGS, &ifr);
  if (err)
    {
      sim_printf ("err devinet_ioctl %d\n", err);
    }

  /* wait for packets */
  void *fiber = sim_fiber_new (&sim_netdev_rx_trampoline, dev, 1 << 16, "NET_RX");
  struct SimTask *task = NULL;
  task = sim_task_create (fiber, getpid ());
  list_add_tail_rcu (&task->head, &g_task_lists);
  sim_fiber_start (fiber);

  return;
}


int (*host_pthread_create)(pthread_t *, const pthread_attr_t *,
    void *(*) (void *), void *) = NULL;

static
void nuse_hijack_init (void)
{
  /* hijacking functions */
  if (!host_pthread_create)
    {
      host_pthread_create = dlsym (RTLD_NEXT, "pthread_create");
      if (!host_pthread_create)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }

}

void __attribute__((constructor))
sim_nuse_init (struct SimExported *exported, const struct SimImported *imported, struct SimKernel *kernel)
{
  // make sure we can call the callbacks
#if 0
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
#endif

  nuse_set_affinity ();

  nuse_hijack_init ();
  rcu_init ();

  void *fiber = sim_fiber_new_from_caller (1 << 16, "init");
  g_sim_main_ctx = sim_task_create (fiber, getpid ());
  list_add_tail_rcu (&g_sim_main_ctx->head, &g_task_lists);

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

  nuse_vif_netmap_init ();

  // finally, put the system in RUNNING state.
  system_state = SYSTEM_RUNNING;

  /* create descriptor table */
  memset (g_fd_table, 0, sizeof (g_fd_table));

  /* create netdev sim%s corresponding to underlying netdevs */
  sim_netdev_create ();
}
