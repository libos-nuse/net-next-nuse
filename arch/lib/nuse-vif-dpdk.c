#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_config.h>
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_byteorder.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include "sim-types.h"
#include "nuse-vif.h"


#define PE(_fmt, ...) {							\
		while (1) {						\
			printf("%s:%d: " _fmt "\n",			\
				__func__, __LINE__, ## __VA_ARGS__);	\
			goto exit;					\
		}							\
	}

#define PO(_fmt, ...) {							\
		while (1) {						\
			printf("%s:%d: " _fmt "\n",			\
				__func__, __LINE__, ## __VA_ARGS__);	\
		}							\
	}

static int dpdk_init = 0;

static const char *ealargs[] = {
	"nuse_vif_dpdk",
	"-c 1",
	"-n 1",
};


extern struct SimDevicePacket lib_dev_create_packet(struct SimDevice *dev,
						    int size);
extern void lib_dev_rx(struct SimDevice *device,
		       struct SimDevicePacket packet);
extern void *lib_dev_get_private(struct SimDevice *);
extern void lib_softirq_wakeup(void);

static const struct rte_eth_rxconf rxconf = {
	.rx_thresh		= {
		.pthresh	= 1,
		.hthresh	= 1,
		.wthresh	= 1,
	},
};

static const struct rte_eth_txconf txconf = {
	.tx_thresh		= {
		.pthresh	= 1,
		.hthresh	= 1,
		.wthresh	= 1,
	},
	.tx_rs_thresh		= 1,
};

#define MAX_PKT_BURST           16
#define MEMPOOL_CACHE_SZ        32
#define MAX_PACKET_SZ           2048
#define MBUF_NUM                512
#define MBUF_SIZ        \
	(MAX_PACKET_SZ + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

#define NUMDESC         256
#define NUMQUEUE        1
#define PORTID          (dpdk->portid)


struct nuse_vif_dpdk {

	int portid;
	struct rte_mempool *rxpool, *txpool;    /* rin buffer pool */

	char txpoolname[16], rxpoolname[16];

	/* burst receive context by rump dpdk code */
	struct rte_mbuf *rms[MAX_PKT_BURST];
	int npkts;
	int bufidx;
};


static inline void
deliverframes(struct nuse_vif_dpdk *dpdk, struct SimDevice *dev)
{
	void *data;
	uint32_t size;
	struct rte_mbuf *rm, *rm0;

	rm0 = dpdk->rms[dpdk->bufidx];
	dpdk->npkts--;
	dpdk->bufidx++;

	for (rm = rm0; rm; rm = rm->pkt.next) {
		struct SimDevicePacket packet;

		data = rte_pktmbuf_mtod(rm, void *);
		size = rte_pktmbuf_data_len(rm);

		packet = lib_dev_create_packet(dev, size);
		memcpy(packet.buffer, data, size);
		lib_dev_rx(dev, packet);
	}

	lib_softirq_wakeup();
}

void
nuse_vif_dpdk_read(struct nuse_vif *vif, struct SimDevice *dev)
{
	struct nuse_vif_dpdk *dpdk = vif->private;

	while (1) {
		/* this burst receive is inspired by RUMP dpdk */

		/* there are packets which are not delivered to SimDevice */
		if (dpdk->npkts > 0)
			while (dpdk->npkts > 0)
				deliverframes(dpdk, dev);

		/* there is no undelivered packet. Check new packets. */
		if (dpdk->npkts == 0) {
			dpdk->npkts = rte_eth_rx_burst(PORTID, 0, dpdk->rms,
						       MAX_PKT_BURST);
			dpdk->bufidx = 0;
		}

		/* XXX: busy wait ??? */
	}
}

void
nuse_vif_dpdk_write(struct nuse_vif *vif, struct SimDevice *dev,
		    unsigned char *data, int len)
{
	void *pkt;
	struct rte_mbuf *rm;
	struct nuse_vif_dpdk *dpdk = vif->private;

	rm = rte_pktmbuf_alloc(dpdk->txpool);
	pkt = rte_pktmbuf_append(rm, len);
	memcpy(pkt, data, len);

	uint16_t ret = rte_eth_tx_burst(PORTID, 0, &rm, 1);
	/* XXX: should be bursted !! */
}


static int
dpdk_if_init(struct nuse_vif_dpdk *dpdk)
{
	int ret;
	struct rte_eth_conf portconf;
	struct rte_eth_link link;

	if (!dpdk_init) {
		ret = rte_eal_init(sizeof(ealargs) / sizeof(ealargs[0]),
				   (void *)(uintptr_t)ealargs);
		if (ret < 0)
			PE("failed to initialize eal");

		ret = -EINVAL;

		ret = rte_eal_pci_probe();
		if (ret < 0)
			PE("eal pci probe failed");

		dpdk_init = 1;
	}

	dpdk->txpool =
		rte_mempool_create(dpdk->txpoolname,
				   MBUF_NUM, MBUF_SIZ, MEMPOOL_CACHE_SZ,
				   sizeof(struct rte_pktmbuf_pool_private),
				   rte_pktmbuf_pool_init, NULL,
				   rte_pktmbuf_init, NULL, 0, 0);

	if (dpdk->txpool == NULL)
		PE("failed to allocate tx pool");


	dpdk->rxpool =
		rte_mempool_create(dpdk->rxpoolname, MBUF_NUM, MBUF_SIZ, 0,
				   sizeof(struct rte_pktmbuf_pool_private),
				   rte_pktmbuf_pool_init, NULL,
				   rte_pktmbuf_init, NULL, 0, 0);

	if (dpdk->rxpool == NULL)
		PE("failed to allocate rx pool");


	memset(&portconf, 0, sizeof(portconf));
	ret = rte_eth_dev_configure(PORTID, NUMQUEUE, NUMQUEUE, &portconf);
	if (ret < 0)
		PE("failed to configure port");


	ret = rte_eth_rx_queue_setup(PORTID, 0, NUMDESC, 0, &rxconf,
				     dpdk->rxpool);

	if (ret < 0)
		PE("failed to setup rx queue");

	ret = rte_eth_tx_queue_setup(PORTID, 0, NUMDESC, 0, &txconf);
	if (ret < 0)
		PE("failed to setup tx queue");

	ret = rte_eth_dev_start(PORTID);
	if (ret < 0)
		PE("failed to start device");

	rte_eth_link_get(PORTID, &link);
	if (!link.link_status)
		PO("interface state is down");

	/* should be promisc ? */
	rte_eth_promiscuous_enable(PORTID);

exit:
	return ret;
}

void *
nuse_vif_dpdk_create(const char *ifname)
{
	struct nuse_vif *vif;
	struct nuse_vif_dpdk *dpdk;

	dpdk = malloc(sizeof(struct nuse_vif_dpdk));
	if (sscanf(ifname, "dpdk%d", &dpdk->portid) <= 0) {
		free(dpdk);
		PO("ifname failure %s", ifname);
		return NULL;
	}
	snprintf(dpdk->txpoolname, 16, "%s%s", "tx", ifname);
	snprintf(dpdk->rxpoolname, 16, "%s%s", "rx", ifname);

	if (dpdk_if_init(dpdk) < 0) {
		free(dpdk);
		PO("failed to init dpdk interface");
		return NULL;
	}


	vif = malloc(sizeof(struct nuse_vif));
	vif->type = NUSE_VIF_DPDK;
	vif->private = dpdk;

	return (void *)vif;
}

void
nuse_vif_dpdk_delete(struct nuse_vif *vif)
{
	struct nuse_vif_dpdk *dpdk = vif->private;

	/* XXX: how to close dpdk...? */

	free(dpdk);
	free(vif);
}

static struct nuse_vif_impl nuse_vif_dpdk = {
	.read	= nuse_vif_dpdk_read,
	.write	= nuse_vif_dpdk_write,
	.create = nuse_vif_dpdk_create,
	.delete = nuse_vif_dpdk_delete,
};


extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

int
nuse_vif_dpdk_init(void)
{
	nuse_vif[NUSE_VIF_DPDK] = &nuse_vif_dpdk;
	return 0;
}
__define_initcall(nuse_vif_dpdk_init, 1);
