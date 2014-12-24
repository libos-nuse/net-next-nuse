/*
 * virtual network interface feature for NUSE
 * Copyright (c) 2015 Hajime Tazaki
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "nuse-hostcalls.h"
#include "nuse-vif.h"

struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

void
nuse_vif_read(struct nuse_vif *vif, struct SimDevice *dev)
{
	struct nuse_vif_impl *impl = nuse_vif[vif->type];

	return impl->read(vif, dev);
}

void
nuse_vif_write(struct nuse_vif *vif, struct SimDevice *dev,
	       unsigned char *data, int len)
{
	struct nuse_vif_impl *impl = nuse_vif[vif->type];

	return impl->write(vif, dev, data, len);
}

static int
nuse_set_if_promisc(const char *ifname)
{
	int fd;
	struct ifreq ifr;

	fd = host_socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

	if (host_ioctl(fd, SIOCGIFFLAGS, &ifr) != 0) {
		printf("failed to get interface status\n");
		return -1;
	}

	ifr.ifr_flags |= IFF_UP | IFF_PROMISC;

	if (host_ioctl(fd, SIOCSIFFLAGS, &ifr) != 0) {
		printf("failed to set interface to promisc\n");
		return -1;
	}

	return 0;
}

void *
nuse_vif_create(enum viftype type, const char *ifname)
{
	struct nuse_vif_impl *impl = nuse_vif[type];

	/* configure promiscus */
	if (type != NUSE_VIF_TAP)
		nuse_set_if_promisc(ifname);

	return impl->create(ifname);
}

void
nuse_vif_delete(struct nuse_vif *vif)
{
	struct nuse_vif_impl *impl = nuse_vif[vif->type];

	return impl->delete(vif);
}

