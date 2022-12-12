/*
 * virtual net_device feature for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#include "sim-init.h"
#include "sim.h"
#include <linux/ethtool.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/ethtool.h>


struct SimDevice {
  struct net_device dev;
  void *priv;
};


static netdev_tx_t
kernel_dev_xmit(struct sk_buff *skb,
    struct net_device *dev)
{
  int err;

  netif_stop_queue(dev);
  /*if (skb->ip_summed == CHECKSUM_PARTIAL) {
    err = skb_checksum_help(skb);
    if (unlikely(err)) {
      pr_err("checksum error (%d)\n", err);
      return 0;
    }
  }*/

  lib_dev_xmit((struct SimDevice *)dev, skb->data, skb->len);
  dev_kfree_skb(skb);
  netif_wake_queue(dev);
  return 0;
}

static u32 always_on(struct net_device *dev)
{
  return 1;
}

static const struct ethtool_ops lib_ethtool_ops = {
  .get_link   = always_on,
};

static const struct net_device_ops lib_dev_ops = {
  .ndo_start_xmit   = kernel_dev_xmit,
  .ndo_set_mac_address  = eth_mac_addr,
};

static void lib_dev_setup(struct net_device *dev)
{  
	dev->mtu                = (16 * 1024) + 20 + 20 + 12;
	dev->hard_header_len    = ETH_HLEN;     /* 14   */
	dev->addr_len           = ETH_ALEN;     /* 6    */
	dev->tx_queue_len       = 0;
	dev->type               = ARPHRD_ETHER;
	dev->flags              = 0;
	/* dev->priv_flags        &= ~IFF_XMIT_DST_RELEASE; */
	dev->features           = 0
          | NETIF_F_CSUM_MASK
				  | NETIF_F_HIGHDMA
				  | NETIF_F_NETNS_LOCAL;
	/* disabled  NETIF_F_TSO NETIF_F_SG  NETIF_F_FRAGLIST NETIF_F_LLTX */
	dev->ethtool_ops        = &lib_ethtool_ops;
	dev->header_ops         = &eth_header_ops;
	dev->netdev_ops         = &lib_dev_ops;
	dev->priv_destructor         = &free_netdev;
}

struct SimDevice *lib_dev_create (const char *iface, void *priv, enum SimDevFlags flags)
{
  printk("Device Setup");
  int err;
  struct SimDevice *dev =
    (struct SimDevice *)alloc_netdev(sizeof(struct SimDevice),
             iface, NET_NAME_UNKNOWN,
             &lib_dev_setup);  
  ether_setup((struct net_device *)dev);

  if (flags & SIM_DEV_NOARP)
    dev->dev.flags |= IFF_NOARP;
  if (flags & SIM_DEV_POINTTOPOINT)
    dev->dev.flags |= IFF_POINTOPOINT;
  if (flags & SIM_DEV_MULTICAST)
    dev->dev.flags |= IFF_MULTICAST;
  if (flags & SIM_DEV_BROADCAST) {
    dev->dev.flags |= IFF_BROADCAST; 
    memset(dev->dev.broadcast, 0xff, 6);
  }
  dev->priv= priv;
  err = register_netdev(&dev->dev);
   
  printk("ifindex : %d",dev->dev.ifindex);
  //lib_start_kernel(&lib_host_ops,"");  
  return dev;  
}


void lib_dev_destroy (struct SimDevice *dev)
{
  unregister_netdev(&dev->dev);
  /* XXX */
  free_netdev(&dev->dev);
}

static int lib_ndo_change_mtu(struct net_device *dev,
			      int new_mtu)
{
	/* called by kernel to change the mtu when the user */
	/* asks for it. */
	/* XXX should forward the set call to ns-3 and wait for */
	/* ns-3 to notify of the change in the function above */
	/* but I am way too tired to do this now. */
	return 0;
}


void lib_dev_set_address (struct SimDevice *dev, unsigned char buffer[6])
{
  struct sockaddr sa;

	sa.sa_family = dev->dev.type;
	lib_memcpy(&sa.sa_data, buffer, 6);
	dev->dev.netdev_ops->ndo_set_mac_address(&dev->dev, &sa);

  /*struct sockaddr sa;
  int ifindex = get_device_ifindex (dev); 
  lib_memcpy(&sa.sa_data, buffer, 6);
  if (dev->dev.type == AF_INET)
  {
    lib_if_set_ipv4 (ifindex, sa, ETH_TLEN);
  }
  else if (dev->dev.type == AF_INET6)
  {
    lib_if_set_ipv6 (ifindex, sa, ETH_ALEN);
  }
  else
  {
    // Notify dev.type doesn't support
  }*/
}

void lib_dev_set_mtu (struct SimDevice *dev, int mtu)
{  
  dev->dev.mtu = mtu;
}


void lib_dev_rx (struct SimDevice *dev, struct SimDevicePacket packet)
{
  struct sk_buff *skb = packet.token;
  struct net_device *device = &dev->dev;

  skb->protocol = eth_type_trans(skb, device);
  skb->ip_summed = CHECKSUM_PARTIAL;

  netif_rx(skb);
}

static int get_hack_size(int size)
{
	/* Note: this hack is coming from nsc */
	/* Bit of a hack... */
	/* Note that the size allocated here effects the offered window
	   somewhat. I've got this heuristic here to try and match up with
	   what we observe on the emulation network and by looking at the
	   driver code of the eepro100. In both cases we allocate enough
	   space for our packet, which  is the important thing. The amount
	   of slack at the end can make linux decide the grow the window
	   differently. This is quite subtle, but the code in question is
	   in the tcp_grow_window function. It checks skb->truesize, which
	   is the size of the skbuff allocated for the incoming data
	   packet -- what we are allocating right now! */
	if (size < 1200)
		return size + 36;
	else if (size <= 1500)
		return 1536;
	else
		return size + 36;
}

struct SimDevicePacket lib_dev_create_packet (struct SimDevice *dev, int size)
{
  struct SimDevicePacket packet;
  int len = get_hack_size(size);
  struct sk_buff *skb = __dev_alloc_skb(len, __GFP_RECLAIM);

  packet.token = skb;
  packet.buffer = skb_put(skb, len);
  return packet;
}

int get_device_ifindex (struct SimDevice *dev)
{
  return dev->dev.ifindex;
}

void *lib_dev_get_private(struct SimDevice *dev)
{
	return dev->priv;
}