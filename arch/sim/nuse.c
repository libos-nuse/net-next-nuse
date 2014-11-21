#include <linux/init.h>         /* initcall_t */
#include <linux/list.h>         /* linked-list */
#include <linux/kernel.h>       /* SYSTEM_BOOTING */
#include <linux/sched.h>        /* struct task_struct */
#include <uapi/linux/route.h>   /* struct rtentry */
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <net/route.h>
#include <net/ipconfig.h>       /* ip_route_iotcl() */
#include <net/net_namespace.h>
#include <bits/pthreadtypes.h>
#include <drivers/base/base.h>
#include <include/linux/idr.h>
#include <include/linux/rcupdate.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include "sim-init.h"
#include "sim-assert.h"
#include "sim.h"
#include "nuse.h"
#include "nuse-hostcalls.h"
#include "nuse-config.h"
#include "nuse-libc.h"


int kptr_restrict __read_mostly;

struct SimTask;
enum system_states system_state = SYSTEM_BOOTING;


int sim_vprintf(const char *str, va_list args)
{
	return vprintf(str, args);
}
void *sim_malloc(unsigned long size)
{
	return malloc(size);
}
void sim_free(void *buffer)
{
	return free(buffer);
}

void *sim_memcpy(void *dst, const void *src, unsigned long size)
{
	return memcpy(dst, src, size);
}
void *sim_memset(void *dst, char value, unsigned long size)
{
	return memset(dst, value, size);
}
__u64 sim_current_ns(void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec * 1000000000 + tp.tv_nsec;
}
unsigned long sim_random(void)
{
	return random();
}

static struct SimTask *g_sim_main_ctx = NULL;
struct list_head g_task_lists = LIST_HEAD_INIT(g_task_lists);

struct SimTask *sim_task_current(void)
{
	struct SimTask *task;

	rcu_read_lock();
	list_for_each_entry_rcu(task, &g_task_lists, head) {
		void *fiber = task->private;

		if (fiber && nuse_fiber_isself(fiber)) {
			rcu_read_unlock();
			return task;
		}
	}
	rcu_read_unlock();

	return g_sim_main_ctx;
}

struct NuseTaskTrampolineContext {
	void (*callback)(void *);
	void *context;
	struct SimTask *task;
};

void
nuse_task_add(void *fiber)
{
	struct SimTask *task = sim_task_create(fiber, getpid());

	list_add_tail_rcu(&task->head, &g_task_lists);
}

static void *nuse_task_start_trampoline(void *context)
{
	/* we use this trampoline solely for the purpose of executing
	   sim_update_jiffies. prior to calling the callback. */
	struct NuseTaskTrampolineContext *ctx = context;
	int found = 0;
	struct SimTask *task;

	void (*callback)(void *);
	void *callback_context;

	list_for_each_entry_rcu(task, &g_task_lists, head) {
		if (task->private == ctx->task->private) {
			found = 1;
			break;
		}
	}
	if (!found)
		return NULL;

	if (nuse_fiber_is_stopped(ctx->task->private)) {
		sim_free(ctx);
		sim_update_jiffies();
		printf("canceled\n");
		return NULL;
	}

	callback = ctx->callback;
	callback_context = ctx->context;
	task = ctx->task;
	sim_free(ctx);
	sim_update_jiffies();

	callback(callback_context);

	/*  nuse_fiber_free (task->private); */
	list_del_rcu(&task->head);

	return ctx;
}

struct SimTask *sim_task_start(void (*callback) (void *), void *context)
{
	struct SimTask *task = NULL;
	struct NuseTaskTrampolineContext *ctx =
		sim_malloc(sizeof(struct NuseTaskTrampolineContext));

	if (!ctx)
		return NULL;
	ctx->callback = callback;
	ctx->context = context;

	void *fiber = nuse_fiber_new(&nuse_task_start_trampoline, ctx, 1 << 16,
				     "task");
	task = sim_task_create(fiber, getpid());
	ctx->task = task;

	if (!nuse_fiber_is_stopped(task->private))
		list_add_tail_rcu(&task->head, &g_task_lists);

	nuse_fiber_start(fiber);
	return task;
}

void *sim_event_schedule_ns(__u64 ns, void (*fn) (void *context), void *context)
{
	struct SimTask *task = NULL;
	struct NuseTaskTrampolineContext *ctx =
		sim_malloc(sizeof(struct NuseTaskTrampolineContext));
	void *fiber;

	if (!ctx)
		return NULL;
	ctx->callback = fn;
	ctx->context = context;

	/* without fiber_start (pthread) */
	fiber = nuse_fiber_new_from_caller(1 << 16, "task_sched");
	task = sim_task_create(fiber, getpid());
	ctx->task = task;

	list_add_tail_rcu(&task->head, &g_task_lists);

	nuse_add_timer(ns, nuse_task_start_trampoline, ctx, fiber);

	return task;
}

void sim_event_cancel(void *event)
{
	struct SimTask *task = event;

	nuse_fiber_stop(task->private);
	/*  nuse_fiber_free (task->private); */
	list_del_rcu(&task->head);
}

void sim_task_wait(void)
{
	struct SimTask *task;

	/* FIXME */
	rcu_sched_qs(0);
	task = sim_task_current();
	sim_assert(task != NULL);
	nuse_fiber_wait(task->private);
	sim_update_jiffies();
}

int sim_task_wakeup(struct SimTask *task)
{
	return nuse_fiber_wakeup(task->private);
}

void *
nuse_netdev_rx_trampoline(void *context)
{
	struct SimDevice *dev = context;
	struct nuse_vif *vif = sim_dev_get_private(dev);

	nuse_vif_read(vif, dev);
	printf("should not reach here %s\n", __func__);
	/* should not reach */
	return dev;
}

void
sim_dev_xmit(struct SimDevice *dev, unsigned char *data, int len)
{
	struct nuse_vif *vif = sim_dev_get_private(dev);

	nuse_vif_write(vif, dev, data, len);
	sim_softirq_wakeup();
}

void sim_signal_raised(struct SimTask *task, int sig)
{
	static int logged = 0;

	if (!logged) {
		sim_printf("%s: Not implemented yet\n", __func__);
		logged = 1;
	}
}

void
nuse_netdev_lo_up(void)
{
	int err;
	static int init_loopback = 0;
	struct ifreq ifr;

	/* loopback IFF_UP */
	if (!init_loopback) {
		memset(&ifr, 0, sizeof(struct ifreq));
		ifr.ifr_flags = IFF_UP;
		sprintf(ifr.ifr_name, "lo");
		err = devinet_ioctl(&init_net, SIOCSIFFLAGS, &ifr);
		if (err)
			sim_printf("err devinet_ioctl %d\n", err);
		init_loopback = 1;
	}
}

void
nuse_netdev_create(struct nuse_vif_config *vifcf)
{
	/* create net_device for nuse process from nuse_vif_config */
	int err;
	struct nuse_vif *vif;
	struct ifreq ifr;
	struct SimTask *task = NULL;
	void *fiber;

	printf("create vif %s\n", vifcf->ifname);
	printf("  address = %s\n", vifcf->address);
	printf("  netmask = %s\n", vifcf->netmask);
	printf("  macaddr = %s\n", vifcf->macaddr);
	printf("  type    = %d\n", vifcf->type);

	vif = nuse_vif_create(vifcf->type, vifcf->ifname);
	if (!vif) {
		sim_printf("vif create error\n");
		sim_assert(0);
	}

	/* create new new_device */
	struct SimDevice *dev = sim_dev_create(vifcf->ifname, vif, 0);

	/* assign new hw address */
	sim_dev_set_address(dev, vifcf->mac);
	ether_setup((struct net_device *)dev);

	/* assign IPv4 address */
	/* XXX: ifr_name is already fileed by nuse_config_parse_interface,
	   I don't know why, but vifcf->ifr_vif_addr.ifr_name is NULL here. */
	strcpy(vifcf->ifr_vif_addr.ifr_name, vifcf->ifname);

	err = devinet_ioctl(&init_net, SIOCSIFADDR, &vifcf->ifr_vif_addr);
	if (err) {
		perror("devinet_ioctl");
		printf("err devinet_ioctl for assign address %s for %s,%s %d\n",
		       vifcf->address, vifcf->ifname,
		       vifcf->ifr_vif_addr.ifr_name, err);
	}

	/* set netmask */
	err = devinet_ioctl(&init_net, SIOCSIFNETMASK, &vifcf->ifr_vif_mask);
	if (err) {
		perror("devinet_ioctl");
		printf("err devinet_ioctl for assign netmask %s for %s,%s %d\n",
		       vifcf->netmask, vifcf->ifname,
		       vifcf->ifr_vif_mask.ifr_name, err);
	}

	/* IFF_UP */
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_UP;
	strncpy(ifr.ifr_name, vifcf->ifname, IFNAMSIZ);

	err = devinet_ioctl(&init_net, SIOCSIFFLAGS, &ifr);
	if (err) {
		perror("devinet_ioctl");
		printf("err devinet_ioctl to set ifup dev %s %d\n",
		       vifcf->ifname, err);
	}

	/* wait for packets */
	fiber = nuse_fiber_new(&nuse_netdev_rx_trampoline, dev,
			       1 << 16, "NET_RX");
	task = sim_task_create(fiber, getpid());
	list_add_tail_rcu(&task->head, &g_task_lists);
	nuse_fiber_start(fiber);
}


void
nuse_route_install(struct nuse_route_config *rtcf)
{
	int err;

	err = ip_rt_ioctl(&init_net, SIOCADDRT, &rtcf->route);
	if (err)
		sim_printf("err ip_rt_ioctl to add route to %s via %s %d\n",
			   rtcf->network, rtcf->gateway, err);
}

void __attribute__((constructor))
nuse_init(void)
{
	int n;
	char *config;
	struct nuse_config cf;
	void *fiber;

	nuse_set_affinity();
	nuse_hijack_init();

	/* create descriptor table */
	memset(nuse_fd_table, 0, sizeof(nuse_fd_table));
	nuse_fd_table[1].real_fd = 1;
	nuse_fd_table[2].real_fd = 2;

	rcu_init();

	fiber = nuse_fiber_new_from_caller(1 << 16, "init");
	g_sim_main_ctx = sim_task_create(fiber, getpid());
	list_add_tail_rcu(&g_sim_main_ctx->head, &g_task_lists);

	/* in drivers/base/core.c (called normally by drivers/base/init.c) */
	devices_init();
	/* in lib/idr.c (called normally by init/main.c) */
	idr_init_cache();

	sim_proc_net_initialize();

	/* and, then, call the normal initcalls */
	initcall_t *call;
	extern initcall_t __initcall_start[], __initcall_end[];

	call = __initcall_start;
	do {
		(*call)();
		call++;
	} while (call < __initcall_end);

	/* finally, put the system in RUNNING state. */
	system_state = SYSTEM_RUNNING;

	/* loopback IFF_UP * / */
	nuse_netdev_lo_up();

	/* read and parse a config file */
	config = getenv("NUSECONF");
	if (config == NULL)
		printf("config file is not specified\n");
	else {
		if (!nuse_config_parse(&cf, config)) {
			printf("parse config file failed\n");
			sim_assert(0);
		}

		/* create netdevs specified by config file */
		for (n = 0; n < cf.vif_cnt; n++)
			nuse_netdev_create(cf.vifs[n]);

		/* setup route entries */
		for (n = 0; n < cf.route_cnt; n++)
			nuse_route_install(cf.routes[n]);
	}
}
