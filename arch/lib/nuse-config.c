/*
 * config file interface for NUSE
 * Copyright (c) 2015 Ryo Nakamura
 *
 * Author: Ryo Nakamura <upa@wide.ad.jp>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "nuse-vif.h"
#include "nuse-config.h"
#include "nuse-libc.h"

static int
strsplit(char *str, char **args, int max)
{
	int argc;
	char *c;

	for (argc = 0, c = str; *c == ' ' || *c == '\t' || *c == '\n'; c++)
		;
	while (*c && argc < max) {
		args[argc++] = c;
		while (*c && *c > ' ')
			c++;
		while (*c && *c <= ' ')
			*c++ = '\0';
	}

	return argc;
}

static int
nuse_config_parse_interface(char *line, FILE *fp, struct nuse_config *cf)
{
	int ret;
	char buf[1024], *p, *args[2];
	struct nuse_vif_config *vifcf;
	struct sockaddr_in *sin;

	memset(buf, 0, sizeof(buf));

	vifcf = (struct nuse_vif_config *)
		malloc(sizeof(struct nuse_vif_config));

	memset(vifcf, 0, sizeof(struct nuse_vif_config));
	vifcf->type = NUSE_VIF_RAWSOCK; /* default */
	strcpy(vifcf->macaddr, "00:00:00:00:00:00"); /* means random */

	strsplit(line, args, sizeof(args));

	strncpy(vifcf->ifname, args[1], IFNAMSIZ);

	while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {

		ret = strsplit(buf, args, sizeof(args));

		if (ret == 0)
			/* no item in the line */
			break;

		if (strncmp(args[0], "address", 7) == 0)
			strncpy(vifcf->address, args[1], NUSE_ADDR_STRLEN);
		if (strncmp(args[0], "netmask", 7) == 0)
			strncpy(vifcf->netmask, args[1], NUSE_ADDR_STRLEN);
		if (strncmp(args[0], "macaddr", 7) == 0)
			strncpy(vifcf->macaddr, args[1], NUSE_MACADDR_STRLEN);
		if (strncmp(args[0], "viftype", 7) == 0) {
			if (strncmp(args[1], "RAW", 3) == 0)
				vifcf->type = NUSE_VIF_RAWSOCK;
			else if (strncmp(args[1], "NETMAP", 6) == 0)
				vifcf->type = NUSE_VIF_NETMAP;
			else if (strncmp(args[1], "TAP", 3) == 0)
				vifcf->type = NUSE_VIF_TAP;
			else if (strncmp(args[1], "DPDK", 4) == 0)
				vifcf->type = NUSE_VIF_DPDK;
			else if (strncmp(args[1], "PIPE", 4) == 0)
				vifcf->type = NUSE_VIF_PIPE;
			else {
				printf("invalid vif type %s\n", args[1]);
				free(vifcf);
				return 0;
			}
		}

		/* mkfifo path for VIF_PIPE */
		if (strncmp(args[0], "pipepath", 4) == 0) {
			strncpy(vifcf->pipepath, args[1], PATH_MAX);
		}
	}

	/* setup ifreq */
	sin = (struct sockaddr_in *)&vifcf->ifr_vif_addr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(vifcf->address);

	sin = (struct sockaddr_in *)&vifcf->ifr_vif_mask.ifr_netmask;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(vifcf->netmask);

	/* XXX: ifname attached to nuse process is same as host stck */
	strncpy(vifcf->ifr_vif_addr.ifr_name, vifcf->ifname, IFNAMSIZ);
	strncpy(vifcf->ifr_vif_mask.ifr_name, vifcf->ifname, IFNAMSIZ);

	/* reassemble mac address */
	if (sscanf(vifcf->macaddr, "%u:%u:%u:%u:%u:%u",
		   (unsigned int *)&vifcf->mac[0],
		   (unsigned int *)&vifcf->mac[1],
		   (unsigned int *)&vifcf->mac[2],
		   (unsigned int *)&vifcf->mac[3],
		   (unsigned int *)&vifcf->mac[4],
		   (unsigned int *)&vifcf->mac[5]) < 1) {
		printf("failed to parse mac address %s\n", vifcf->macaddr);
		free(vifcf);
		return 0;
	}

	cf->vifs[cf->vif_cnt++] = vifcf;

	return 1;
}

static int
nuse_config_parse_route(char *line, FILE *fp, struct nuse_config *cf)
{
	int ret, net, mask, gate;
	char buf[1024], *p, *args[2];
	struct nuse_route_config *rtcf;
	struct sockaddr_in *sin;

	net = 0;
	mask = 0;
	gate = 0;

	memset(buf, 0, sizeof(buf));

	rtcf = (struct nuse_route_config *)
	       malloc(sizeof(struct nuse_route_config));
	if (!rtcf)
		assert(0);

	memset(rtcf, 0, sizeof(struct nuse_route_config));

	while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {

		ret = strsplit(buf, args, sizeof(args));

		if (ret == 0)
			/* no item in the line */
			break;

		if (strncmp(args[0], "network", 7) == 0) {
			strncpy(rtcf->network, args[1], NUSE_ADDR_STRLEN);
			net = 1;
		}
		if (strncmp(args[0], "netmask", 7) == 0) {
			strncpy(rtcf->netmask, args[1], NUSE_ADDR_STRLEN);
			mask = 1;
		}
		if (strncmp(args[0], "gateway", 7) == 0) {
			strncpy(rtcf->gateway, args[1], NUSE_ADDR_STRLEN);
			gate = 1;
		}

		if (net && mask && gate)
			break;
	}

	if (!net)
		printf("network is not configured !\n");
	if (!mask)
		printf("netmask is not configured !\n");
	if (!gate)
		printf("netmask is not configured !\n");

	if (!net || !mask || !gate) {
		free (rtcf);
		return 0;
	}

	/* setup rtentry */
	sin = (struct sockaddr_in *)&rtcf->route.rt_dst;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(rtcf->network);

	sin = (struct sockaddr_in *)&rtcf->route.rt_genmask;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(rtcf->netmask);

	sin = (struct sockaddr_in *)&rtcf->route.rt_gateway;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(rtcf->gateway);

	rtcf->route.rt_flags = RTF_UP | RTF_GATEWAY;
	rtcf->route.rt_metric = 0;

	cf->routes[cf->route_cnt++] = rtcf;

	return 1;
}


int
nuse_config_parse(struct nuse_config *cf, char *cfname)
{
	FILE *fp;
	int ret = 0;
	char buf[1024];

	memset(cf, 0, sizeof(struct nuse_config));
	fp = fopen(cfname, "r");
	if (fp == NULL) {
		perror("fopen");
		return 0;
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (strncmp(buf, "interface ", 10) == 0)
			ret = nuse_config_parse_interface(buf, fp, cf);
		else if (strncmp(buf, "route", 5) == 0)
			ret = nuse_config_parse_route(buf, fp, cf);
		else
			continue;
		if (!ret)
			break;
	}

	fclose(fp);

	return ret;
}

void
nuse_config_free(struct nuse_config *cf)
{
	int n;

	for (n = 0; n < cf->vif_cnt; n++)
		free(cf->vifs[n]);

	for (n = 0; n < cf->route_cnt; n++)
		free(cf->routes[n]);
}

