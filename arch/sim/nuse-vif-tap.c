#include <linux/if_tun.h>
#include <linux/netdevice.h>
#include <stdio.h>


#include "nuse-hostcalls.h"
#include "nuse.h"
#include "sim.h"
#include "sim-types.h"

static int
tap_alloc(const char *dev)
{
	/* create tunnel interface */
	int fd;
	struct ifreq ifr;

	fd = host_open("/dev/net/tun", O_RDWR, 0);
	if (fd < 0) {
		printf("failed to open /dev/tun\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", dev);

	if (host_ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
		printf("failed to create tap interface \"%s\"\n",
		       ifr.ifr_name);
		return -1;
	}

	printf("%s: alloc tap interface %s\n", __func__, dev);

	return fd;
}


int
tap_up(char *dev)
{
	int udp_fd;
	struct ifreq ifr;

	/* Make Tunnel interface up state */
	udp_fd = host_socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_fd < 0) {
		printf("failt to create control socket of tap interface.\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_UP;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if (host_ioctl(udp_fd, SIOCSIFFLAGS, (void *)&ifr) < 0) {
		printf("failed to make %s up.\n", dev);
		host_close(udp_fd);
		return -1;
	}

	host_close(udp_fd);

	return 1;
}

void
nuse_vif_tap_read(struct nuse_vif *vif, struct SimDevice *dev)
{
	int sock = vif->sock;
	char buf[8192];
	ssize_t size;
	struct SimDevicePacket packet;

	while (1) {
		size = host_read(sock, buf, sizeof(buf));
		if (size < 0) {
			perror("read");
			host_close(sock);
			return;
		} else if (size == 0) {
			host_close(sock);
			return;
		}

		struct ethhdr {
			unsigned char h_dest[6];
			unsigned char h_source[6];
			uint16_t h_proto;
		} *hdr = (struct ethhdr *)buf;

		/* check dest mac for promisc */
		if (memcmp(hdr->h_dest,
			   ((struct net_device *)dev)->dev_addr, 6) &&
		    memcmp(hdr->h_dest,
			   ((struct net_device *)dev)->broadcast, 6)) {
			sim_softirq_wakeup();
			continue;
		}

		packet = sim_dev_create_packet(dev, size);
		/* XXX: FIXME should not copy */
		memcpy(packet.buffer, buf, size);

		sim_dev_rx(dev, packet);
		sim_softirq_wakeup();
	}
}

void
nuse_vif_tap_write(struct nuse_vif *vif, struct SimDevice *dev,
		   unsigned char *data, int len)
{
	int sock = vif->sock;
	int ret = host_write(sock, data, len);

	if (ret == -1)
		perror("write");
}


void *
nuse_vif_tap_create(const char *ifname)
{
	int sock;
	char dev[IFNAMSIZ];
	struct nuse_vif *vif;

	snprintf(dev, sizeof(dev), "nuse-%s", ifname);

	sock = tap_alloc(dev);
	if (sock < 0) {
		perror("tap_alloc");
		return NULL;
	}

	if (tap_up(dev) < 0) {
		perror("tap_up");
		return NULL;
	}

	vif = sim_malloc(sizeof(struct nuse_vif));
	vif->sock = sock;
	vif->type = NUSE_VIF_TAP;

	return vif;
}

void
nuse_vif_tap_delete(struct nuse_vif *vif)
{
	int sock = vif->sock;

	host_close(sock);
}

static struct nuse_vif_impl nuse_vif_tap = {
	.read	= nuse_vif_tap_read,
	.write	= nuse_vif_tap_write,
	.create = nuse_vif_tap_create,
	.delete = nuse_vif_tap_delete,
};


extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

int nuse_vif_tap_init(void)
{
	nuse_vif[NUSE_VIF_TAP] = &nuse_vif_tap;
	return 0;
}
__define_initcall(nuse_vif_tap_init, 1);
