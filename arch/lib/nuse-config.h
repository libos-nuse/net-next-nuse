/*
 * config file interface for NUSE
 * Copyright (c) 2015 Ryo Nakamura
 *
 * Author: Ryo Nakamura <upa@wide.ad.jp>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef _NUSE_CONFIG_H_
#define _NUSE_CONFIG_H_

#include <linux/limits.h>	/* PATH_MAX */
#include <linux/if_ether.h>
#include <linux/route.h>


#define NUSE_VIF_MAX            16
#define NUSE_ROUTE_MAX          16
#define NUSE_ADDR_STRLEN        16
#define NUSE_MACADDR_STRLEN     20


struct nuse_vif_config {
	char ifname[IFNAMSIZ];
	char address[NUSE_ADDR_STRLEN];
	char netmask[NUSE_ADDR_STRLEN];
	char macaddr[NUSE_MACADDR_STRLEN];

	enum viftype type;

	u_char mac[ETH_ALEN];

	struct ifreq ifr_vif_addr;
	struct ifreq ifr_vif_mask;

	/* for vif-pipe */
	char pipepath[PATH_MAX];
};

struct nuse_route_config {
	char network[NUSE_ADDR_STRLEN];
	char netmask[NUSE_ADDR_STRLEN];
	char gateway[NUSE_ADDR_STRLEN];

	struct rtentry route;
};

struct nuse_config {
	int vif_cnt, route_cnt;
	struct nuse_vif_config *vifs[NUSE_VIF_MAX];
	struct nuse_route_config *routes[NUSE_ROUTE_MAX];
};


/* open cfname and return struct  nuse_config */
int nuse_config_parse(struct nuse_config *cf, char *cfname);

/* free cf->nuse_vif_config and cf->nuse_route_config  */
void nuse_config_free(struct nuse_config *cf);



#endif /* _NUSE_CONFIG_H_ */
