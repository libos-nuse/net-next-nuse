#include <linux/init.h>         /* initcall_t */
#include <linux/list.h>         /* linked-list */
#include <linux/kernel.h>       /* SYSTEM_BOOTING */
#include <linux/sched.h>        /* struct task_struct */
#include <uapi/linux/route.h>   /* struct rtentry */
#include <linux/netdevice.h>
#include <linux/etherdevice.h>	/* eth_random_addr() */
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
#include <dlfcn.h>
#include "sim-init.h"
#include "sim-assert.h"
#include "sim.h"
#include "nuse.h"
#include "nuse-hostcalls.h"
#include "nuse-vif.h"
#include "nuse-config.h"
#include "nuse-libc.h"


struct SimTask;


int nuse_vprintf(struct SimKernel *kernel, const char *str, va_list args)
{
	return vprintf(str, args);
}
void *nuse_malloc(struct SimKernel *kernel, unsigned long size)
{
	return malloc(size);
}
void nuse_free(struct SimKernel *kernel, void *buffer)
{
	return free(buffer);
}

void *nuse_memcpy(struct SimKernel *kernel, void *dst, const void *src,
		unsigned long size)
{
	return memcpy(dst, src, size);
}
void *nuse_memset(struct SimKernel *kernel, void *dst, char value,
		unsigned long size)
{
	return memset(dst, value, size);
}
__u64 nuse_current_ns(struct SimKernel *kernel)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec * 1000000000 + tp.tv_nsec;
}
unsigned long nuse_random(struct SimKernel *kernel)
{
	return random();
}
char *nuse_getenv(struct SimKernel *kernel, const char *name)
{
	return host_getenv(name);
}
int nuse_fclose(struct SimKernel *kernel, FILE *fp)
{
	return host_fclose(fp);
}
size_t nuse_fwrite(struct SimKernel *kernel, const void *ptr,
		size_t size, size_t nmemb, FILE *stream)
{
	return host_fwrite(ptr, size, nmemb, stream);
}
int nuse_access(struct SimKernel *kernel, const char *pathname, int mode)
{
	return host_access(pathname, mode);
}

static struct SimTask *g_nuse_main_ctx = NULL;
struct list_head g_task_lists = LIST_HEAD_INIT(g_task_lists);

struct SimTask *nuse_task_current(struct SimKernel *kernel)
{
	struct SimTask *task;
	void *fiber;

	rcu_read_lock();
	list_for_each_entry_rcu(task, &g_task_lists, head) {
		void *fiber = task->private;

		if (fiber && nuse_fiber_isself(fiber)) {
			rcu_read_unlock();
			return task;
		}
	}
	rcu_read_unlock();

	if (!g_nuse_main_ctx) {
		fiber = nuse_fiber_new_from_caller(1 << 16, "init");
		g_nuse_main_ctx = lib_task_create(fiber, getpid());
		list_add_tail_rcu(&g_nuse_main_ctx->head, &g_task_lists);
	}
	return g_nuse_main_ctx;
}

struct NuseTaskTrampolineContext {
	void (*callback)(void *);
	void *context;
	struct SimTask *task;
};

void
nuse_task_add(void *fiber)
{
	struct SimTask *task = lib_task_create(fiber, getpid());

	list_add_tail_rcu(&task->head, &g_task_lists);
}

static void *nuse_task_start_trampoline(void *context)
{
	/* we use this trampoline solely for the purpose of executing
	   lib_update_jiffies. prior to calling the callback. */
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
		lib_free(ctx);
		lib_update_jiffies();
		printf("canceled\n");
		return NULL;
	}

	callback = ctx->callback;
	callback_context = ctx->context;
	task = ctx->task;
	lib_free(ctx);
	lib_update_jiffies();

	callback(callback_context);

	/*  nuse_fiber_free (task->private); */
	list_del_rcu(&task->head);

	return ctx;
}

struct SimTask *nuse_task_start(struct SimKernel *kernel, 
				void (*callback) (void *), void *context)
{
	struct SimTask *task = NULL;
	struct NuseTaskTrampolineContext *ctx =
		lib_malloc(sizeof(struct NuseTaskTrampolineContext));

	if (!ctx)
		return NULL;
	ctx->callback = callback;
	ctx->context = context;

	void *fiber = nuse_fiber_new(&nuse_task_start_trampoline, ctx, 1 << 16,
				     "task");
	task = lib_task_create(fiber, getpid());
	ctx->task = task;

	if (!nuse_fiber_is_stopped(task->private))
		list_add_tail_rcu(&task->head, &g_task_lists);

	nuse_fiber_start(fiber);
	return task;
}

void *nuse_event_schedule_ns(struct SimKernel *kernel,
			__u64 ns, void (*fn) (void *context), void *context,
			void (*dummy_fn)(void))
{
	struct SimTask *task = NULL;
	struct NuseTaskTrampolineContext *ctx =
		lib_malloc(sizeof(struct NuseTaskTrampolineContext));
	void *fiber;

	if (!ctx)
		return NULL;
	ctx->callback = fn;
	ctx->context = context;

	/* without fiber_start (pthread) */
	fiber = nuse_fiber_new_from_caller(1 << 16, "task_sched");
	task = lib_task_create(fiber, getpid());
	ctx->task = task;

	list_add_tail_rcu(&task->head, &g_task_lists);

	nuse_add_timer(ns, nuse_task_start_trampoline, ctx, fiber);

	return task;
}

void nuse_event_cancel(struct SimKernel *kernel, void *event)
{
	struct SimTask *task = event;

	nuse_fiber_stop(task->private);
	/*  nuse_fiber_free (task->private); */
	list_del_rcu(&task->head);
}

void nuse_task_wait(struct SimKernel *kernel)
{
	struct SimTask *task;

	/* FIXME */
	rcu_sched_qs();
	task = nuse_task_current(NULL);
	lib_assert(task != NULL);
	nuse_fiber_wait(task->private);
	lib_update_jiffies();
}

int nuse_task_wakeup(struct SimKernel *kernel, struct SimTask *task)
{
	return nuse_fiber_wakeup(task->private);
}

void *
nuse_netdev_rx_trampoline(void *context)
{
	struct SimDevice *dev = context;
	struct nuse_vif *vif = lib_dev_get_private(dev);

	nuse_vif_read(vif, dev);
	printf("should not reach here %s\n", __func__);
	/* should not reach */
	return dev;
}

void
nuse_dev_rx(struct SimDevice *dev, char *buf, int size)
{
	struct ethhdr {
		unsigned char h_dest[6];
		unsigned char h_source[6];
		uint16_t h_proto;
	} *hdr = (struct ethhdr *)buf;

	/* check dest mac for promisc */
	if (memcmp(hdr->h_dest, ((struct net_device *)dev)->dev_addr, 6) &&
		memcmp(hdr->h_dest, ((struct net_device *)dev)->broadcast,6)) {
		lib_softirq_wakeup();
		return;
	}

	struct SimDevicePacket packet = lib_dev_create_packet(dev, size);
	/* XXX: FIXME should not copy */
	memcpy(packet.buffer, buf, size);

	lib_dev_rx(dev, packet);
	lib_softirq_wakeup();
}

void
nuse_dev_xmit(struct SimKernel *kernel, struct SimDevice *dev,
	unsigned char *data, int len)
{
	struct nuse_vif *vif = lib_dev_get_private(dev);

	nuse_vif_write(vif, dev, data, len);
	lib_softirq_wakeup();
}

void nuse_signal_raised(struct SimKernel *kernel, struct SimTask *task, int sig)
{
	static int logged = 0;

	if (!logged) {
		lib_printf("%s: Not implemented yet\n", __func__);
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
			lib_printf("err devinet_ioctl %d\n", err);
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
		lib_printf("vif create error\n");
		lib_assert(0);
	}

	/* create new new_device */
	struct SimDevice *dev = lib_dev_create(vifcf->ifname, vif, 0);

	/* assign new hw address */
	if (vifcf->mac[0] == 0 && vifcf->mac[1] == 0 && vifcf->mac[2] == 0 &&
	    vifcf->mac[3] == 0 && vifcf->mac[4] == 0 && vifcf->mac[5] == 0) {
		eth_random_addr(vifcf->mac);
		((struct net_device *)dev)->addr_assign_type = NET_ADDR_RANDOM;
		printf("mac address for %s is randomized ", vifcf->ifname);
		printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
		       vifcf->mac[0], vifcf->mac[1], vifcf->mac[2],
		       vifcf->mac[3], vifcf->mac[4], vifcf->mac[5]);
	}
	lib_dev_set_address(dev, vifcf->mac);
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
	task = lib_task_create(fiber, getpid());
	list_add_tail_rcu(&task->head, &g_task_lists);
	nuse_fiber_start(fiber);
}


void
nuse_route_install(struct nuse_route_config *rtcf)
{
	int err;

	err = ip_rt_ioctl(&init_net, SIOCADDRT, &rtcf->route);
	if (err)
		lib_printf("err ip_rt_ioctl to add route to %s via %s %d\n",
			   rtcf->network, rtcf->gateway, err);
}

/* XXX */
int nuse_open(struct SimKernel *kernel, const char *pathname,
	int flags, ...);

extern void lib_init(struct SimExported *exported,
		const struct SimImported *imported,
		struct SimKernel *kernel);

void __attribute__((constructor))
nuse_init(void)
{
	int n;
	char *config;
	struct nuse_config cf;

	nuse_hostcall_init();
	nuse_set_affinity();

	/* create descriptor table */
	memset(nuse_fd_table, 0, sizeof(nuse_fd_table));
	nuse_fd_table[1].real_fd = 1;
	nuse_fd_table[2].real_fd = 2;

	/* are those rump hypercalls? */
	struct SimImported *imported = malloc(sizeof(struct SimImported));
	memset(imported, 0, sizeof(struct SimImported));
	imported->vprintf = nuse_vprintf;
	imported->malloc = nuse_malloc;
	imported->free = nuse_free;
	imported->memcpy = nuse_memcpy;
	imported->memset = nuse_memset;
	imported->atexit = NULL; /* not implemented */
	imported->access = nuse_access;
	imported->getenv = nuse_getenv;
	imported->mkdir = NULL; /* not implemented */
	/* it's not hypercall, but just a POSIX glue ? */
	imported->open = nuse_open;
	imported->__fxstat = NULL; /* not implemented */
	imported->fseek = NULL; /* not implemented */
	imported->setbuf = NULL; /* not implemented */
	imported->ftell = NULL; /* not implemented */
	imported->fdopen = NULL; /* not implemented */
	imported->fread = NULL; /* not implemented */
	imported->fwrite = nuse_fwrite;
	imported->fclose = nuse_fclose;
	imported->random = nuse_random;
	imported->event_schedule_ns = nuse_event_schedule_ns;
	imported->event_cancel = nuse_event_cancel;
	imported->current_ns = nuse_current_ns;
	imported->task_start = nuse_task_start;
	imported->task_wait = nuse_task_wait;
	imported->task_current = nuse_task_current;
	imported->task_wakeup = nuse_task_wakeup;
	imported->task_yield = NULL; /* not implemented */
	imported->dev_xmit = nuse_dev_xmit;
	imported->signal_raised = nuse_signal_raised;
	imported->poll_event = NULL; /* not implemented */

	struct SimExported *exported = malloc(sizeof(struct SimExported));
	lib_init (exported, imported, NULL);

	/* XXX: trick, reopen stderr */
	stderr = host_fdopen (2, "rw");

	/* loopback IFF_UP * / */
	nuse_netdev_lo_up();

	/* read and parse a config file */
	config = host_getenv("NUSECONF");
	if (config == NULL)
		printf("config file is not specified\n");
	else {
		if (!nuse_config_parse(&cf, config)) {
			printf("parse config file failed\n");
			lib_assert(0);
		}

		/* create netdevs specified by config file */
		for (n = 0; n < cf.vif_cnt; n++)
			nuse_netdev_create(cf.vifs[n]);

		/* setup route entries */
		for (n = 0; n < cf.route_cnt; n++)
			nuse_route_install(cf.routes[n]);
	}
}
