#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <assert.h>

#include <sys/mman.h>
#include <net/netmap.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

#include "sim-types.h"
#include "sim-assert.h"
#include "nuse-vif.h"
#include "nuse-hostcalls.h"


static int vale_rings = 0;

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

struct nuse_vif_netmap {
	int fd;
	struct netmap_if *nifp;
};

extern struct SimDevicePacket lib_dev_create_packet(struct SimDevice *dev,
						    int size);
extern void lib_dev_rx(struct SimDevice *device, struct SimDevicePacket packet);
extern void *lib_dev_get_private(struct SimDevice *);
extern void lib_softirq_wakeup(void);

#define BURST_MAX 1024



static int
netmap_get_nifp(const char *ifname, struct netmap_if **_nifp)
{
	int fd;
	char *mem;
	struct nmreq nmr;
	struct netmap_if *nifp;

	/* open netmap for  ring */

	fd = open("/dev/netmap", O_RDWR);
	if (fd < 0) {
		printf("unable to open /dev/netmap");
		return 0;
	}

	memset(&nmr, 0, sizeof(nmr));
	strcpy(nmr.nr_name, ifname);
	nmr.nr_version = NETMAP_API;
	nmr.nr_ringid = 0 | (NETMAP_NO_TX_POLL | NETMAP_DO_RX_POLL);
	nmr.nr_flags |= NR_REG_ALL_NIC;

	if (ioctl(fd, NIOCREGIF, &nmr) < 0) {
		printf("unable to register interface %s", ifname);
		return 0;
	}

	if (vale_rings && strncmp(ifname, "vale", 4) == 0) {
		nmr.nr_rx_rings = vale_rings;
		nmr.nr_tx_rings = vale_rings;
	}

	mem = mmap(NULL, nmr.nr_memsize,
		   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
		printf("unable to mmap");
		return 0;
	}

	nifp = NETMAP_IF(mem, nmr.nr_offset);
	*_nifp = nifp;

	return fd;
}

void
nuse_vif_netmap_read(struct nuse_vif *vif, struct SimDevice *dev)
{
	int n;
	uint32_t burst, m, rx, cur, size;
	struct pollfd x[1];
	struct netmap_slot *rs;
	struct netmap_ring *ring;
	struct nuse_vif_netmap *netmap = vif->private;
	struct netmap_if *nifp = netmap->nifp;

	x[0].fd = netmap->fd;
	x[0].events = POLLIN;

	while (1) {

		host_poll(x, 1, -1);

		for (n = 0; n < nifp->ni_rx_rings; n++) {
			ring = NETMAP_RXRING(nifp, n);

			if (nm_ring_empty(ring))
				continue;

			m = nm_ring_space(ring);
			m = MIN(m, BURST_MAX);

			cur = ring->cur;

			for (rx = 0; rx < m; rx++) {
				rs = &ring->slot[cur];
				char *p = NETMAP_BUF(ring, rs->buf_idx);

				size = rs->len;

				struct ethhdr {
					unsigned char h_dest[6];
					unsigned char h_source[6];
					uint16_t h_proto;
				} *hdr = (struct ethhdr *)p;

#if DEBUG
				printf("proto = 0x%x, dst= %X:%X:%X:%X:%X:%X\n",
				       ntohs(
					       hdr->h_proto),
				       hdr->h_dest[0], hdr->h_dest[1],
				       hdr->h_dest[2],
				       hdr->h_dest[3], hdr->h_dest[4],
				       hdr->h_dest[5]);
#endif

				struct SimDevicePacket packet =
					lib_dev_create_packet(dev, size);

				/* XXX: FIXME should not copy */
				memcpy(packet.buffer, hdr, size);
				lib_dev_rx(dev, packet);

				cur = nm_ring_next(ring, cur);
			}
			lib_softirq_wakeup();

			ring->head = ring->cur = cur;
		}
	}
}

void
nuse_vif_netmap_write(struct nuse_vif *vif, struct SimDevice *dev,
		      unsigned char *data, int len)
{
	int n;
	uint32_t m, cur, size;
	struct netmap_slot *ts;
	struct netmap_ring *ring;
	struct nuse_vif_netmap *netmap = vif->private;
	struct netmap_if *nifp = netmap->nifp;

	/* XXX make it be parallel */
	ring = NETMAP_TXRING(nifp, 0);

	if (nm_ring_empty(ring))
		return;

	cur = ring->cur;
	ts = &ring->slot[cur];
	memcpy(NETMAP_BUF(ring, ts->buf_idx), data, len);
	ts->len = len;

	cur = nm_ring_next(ring, cur);
	ring->head = ring->cur = cur;

	ioctl(netmap->fd, NIOCTXSYNC, NULL);
}

void *
nuse_vif_netmap_create(const char *ifname)
{

	struct nuse_vif *vif;
	struct nuse_vif_netmap *netmap;

	netmap = (struct nuse_vif_netmap *)malloc(sizeof(*netmap));
	memset(netmap, 0, sizeof(struct nuse_vif_netmap));

	vif = malloc(sizeof(struct nuse_vif));
	memset(vif, 0, sizeof(struct nuse_vif));
	vif->type = NUSE_VIF_NETMAP;
	vif->private = netmap;

	netmap->fd = netmap_get_nifp(ifname, &netmap->nifp);
	if (!netmap->fd) {
		printf("failed to open netmap for \"%s\"\n", ifname);
		free(netmap);
		free(vif);
		assert(0);
	}

	return vif;
}

void
nuse_vif_netmap_delete(struct nuse_vif *vif)
{
	struct nuse_vif_netmap *netmap = vif->private;

	close(netmap->fd);
	free(netmap);
	free(vif);
}

static struct nuse_vif_impl nuse_vif_netmap = {
	.read	= nuse_vif_netmap_read,
	.write	= nuse_vif_netmap_write,
	.create = nuse_vif_netmap_create,
	.delete = nuse_vif_netmap_delete,
};

extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

int nuse_vif_netmap_init(void)
{
	nuse_vif[NUSE_VIF_NETMAP] = &nuse_vif_netmap;
	return 0;
}
__define_initcall(nuse_vif_netmap_init, 1);
