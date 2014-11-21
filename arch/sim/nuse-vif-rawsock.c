#include <net/sock.h>
#include <linux/netdevice.h>
#define _GNU_SOURCE /* Get RTLD_NEXT */
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "sim-init.h"
#include "sim-assert.h"
#include "nuse-hostcalls.h"
#include "nuse.h"
#include "nuse-libc.h"
#include "sim.h"

void
nuse_vif_raw_read(struct nuse_vif *vif, struct SimDevice *dev)
{
	int sock = vif->sock;
	char buf[8192];
	ssize_t size;

	while (1) {
		size = host_read(sock, buf, sizeof(buf));
		if (size < 0) {
			printf("read error errno=%d\n", errno);
			host_close(sock);
			return;
		} else if (size == 0) {
			printf("read error: closed. errno=%d\n", errno);
			host_close(sock);
			return;
		}

		struct ethhdr {
			unsigned char h_dest[6];
			unsigned char h_source[6];
			uint16_t h_proto;
		} *hdr = (struct ethhdr *)buf;

		/* check dest mac for promisc */
		if (memcmp(hdr->h_dest, ((struct net_device *)dev)->dev_addr,
			   6) &&
		    memcmp(hdr->h_dest, ((struct net_device *)dev)->broadcast,
			   6)) {
			sim_softirq_wakeup();
			continue;
		}

		struct SimDevicePacket packet =
			sim_dev_create_packet(dev, size);
		/* XXX: FIXME should not copy */
		memcpy(packet.buffer, buf, size);

		sim_dev_rx(dev, packet);
		sim_softirq_wakeup();
	}

	printf("%s: should not reach", __func__);
}

void
nuse_vif_raw_write(struct nuse_vif *vif, struct SimDevice *dev,
		   unsigned char *data, int len)
{
	int sock = vif->sock;
	int ret = host_write(sock, data, len);

	if (ret == -1)
		perror("write");
}

void *
nuse_vif_raw_create(const char *ifname)
{
	int err;
	int sock = host_socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	struct sockaddr_ll ll;
	struct nuse_vif *vif;

	if (sock < 0)
		perror("socket");

	memset(&ll, 0, sizeof(ll));

	ll.sll_family = AF_PACKET;
	ll.sll_ifindex = if_nametoindex(ifname);
	ll.sll_protocol = htons(ETH_P_ALL);
	err = host_bind(sock, (struct sockaddr *)&ll, sizeof(ll));
	if (err)
		perror("bind");

	vif = sim_malloc(sizeof(struct nuse_vif));
	vif->sock = sock;
	vif->type = NUSE_VIF_RAWSOCK; /* FIXME */
	return vif;
}

void
nuse_vif_raw_delete(struct nuse_vif *vif)
{
	int sock = vif->sock;

	host_close(sock);
}

static struct nuse_vif_impl nuse_vif_rawsock = {
	.read	= nuse_vif_raw_read,
	.write	= nuse_vif_raw_write,
	.create = nuse_vif_raw_create,
	.delete = nuse_vif_raw_delete,
};

extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

static int __init nuse_vif_rawsock_init(void)
{
	nuse_vif[NUSE_VIF_RAWSOCK] = &nuse_vif_rawsock;
	return 0;
}
core_initcall(nuse_vif_rawsock_init);
