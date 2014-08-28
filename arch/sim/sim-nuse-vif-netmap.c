#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <sys/mman.h>
#include <net/netmap.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

//#include "sim-init.h"
#include "sim-types.h"
#include "sim-assert.h"
#include "sim-nuse-vif.h"
//#include "sim.h"


#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
typedef int (*initcall_t)(void);
#define __define_initcall(fn, id)      \
  static initcall_t __initcall_##fn##id                \
       __attribute__((__section__(".initcall" #id ".init"))) = fn;


struct nuse_vif_netmap
{
  struct nm_desc *nmd;
  int fd;
};

extern struct SimDevicePacket sim_dev_create_packet (struct SimDevice *dev, int size);
extern void sim_dev_rx (struct SimDevice *device, struct SimDevicePacket packet);
extern void *sim_dev_get_private (struct SimDevice *);
extern void sim_softirq_wakeup (void);
extern void *sim_malloc (unsigned long size);

#define BURST_MAX 1024

void
nuse_vif_netmap_read (struct nuse_vif *vif, struct SimDevice *dev)
{
  struct nuse_vif_netmap *netmap = vif->private;
  struct netmap_ring *rxring;
  struct pollfd pfd = { .fd = netmap->fd, .events = POLLIN };
  struct netmap_if *nifp = netmap->nmd->nifp;

  uint32_t i, cur, rx, n, size;

  while (1)
    {
      if (host_poll (&pfd, 1, 0) == 0)
        continue;

      if (pfd.revents & POLLERR) {
	printf ("poll err\n");
	sim_assert (0);
      }

      for (i = netmap->nmd->first_rx_ring; i <= netmap->nmd->last_rx_ring; i++) {
	rxring = NETMAP_RXRING (nifp, i);
	if (nm_ring_empty (rxring))
	  continue;

        cur = rxring->cur;
        n = nm_ring_space (rxring);
        for (rx = 0; rx < n; rx++) {
	  struct netmap_slot *slot = &rxring->slot[cur];
	  char *p = NETMAP_BUF (rxring, slot->buf_idx);

	  size = slot->len;
	  struct ethhdr
	  {
	    unsigned char   h_dest[6];
	    unsigned char   h_source[6];
	    uint16_t        h_proto;
	  } *hdr = (struct ethhdr *) p;

#if DEBUG
	  printf ("proto = 0x%x, dst= %X:%X:%X:%X:%X:%X\n",
		  ntohs (hdr->h_proto), hdr->h_dest[0], hdr->h_dest[1], hdr->h_dest[2],
		  hdr->h_dest[3], hdr->h_dest[4],hdr->h_dest[5]);
#endif

	  struct SimDevicePacket packet = sim_dev_create_packet (dev, size);
	  /* XXX: FIXME should not copy */
	  memcpy (packet.buffer, hdr, size);
	  sim_dev_rx (dev, packet);
	  sim_softirq_wakeup ();

	  cur = nm_ring_next (rxring, cur);
        }
        rxring->head = rxring->cur = cur;
      }
    }
  return;
}

void
nuse_vif_netmap_write (struct nuse_vif *vif, struct SimDevice *dev, 
                unsigned char *data, int len)
{
  struct nuse_vif_netmap *netmap = vif->private;

  struct pollfd pfd = { .fd = netmap->fd, .events = POLLOUT };
  struct netmap_if *nifp = netmap->nmd->nifp;
  struct netmap_ring *txring;

  uint8_t *dst;
  uint32_t i, cur, nm_frag_size, offset, last;

  while (1)
    {
      if (host_poll (&pfd, 1, 0) == 0)
	{
	  perror ("poll");
	  continue;
	}
      break;
    }

  for (i = netmap->nmd->first_tx_ring; i <= netmap->nmd->last_tx_ring; i++) {
    if (len <= 0)
      break;

    txring = NETMAP_TXRING (nifp, i);
    offset = 0;
    cur = txring->cur;

    if (nm_ring_empty (txring))
      continue;

    nm_frag_size = MIN (len, txring->nr_buf_size);
    dst = (uint8_t *)NETMAP_BUF (txring, txring->slot[cur].buf_idx);
    txring->slot[cur].len = nm_frag_size;
    txring->slot[cur].flags = NS_MOREFRAG;
    memcpy (dst, data + offset, nm_frag_size);

    last = cur;
    cur = nm_ring_next(txring, cur);

    offset += nm_frag_size;
    len -= nm_frag_size;
  }
  /* The last slot must not have NS_MOREFRAG set. */
  txring->slot[last].flags &= ~NS_MOREFRAG;

  /* Now update ring->cur and ring->head. */
  txring->cur = txring->head = cur;
  ioctl(netmap->fd, NIOCTXSYNC, NULL);

  return;
}


void *
nuse_vif_netmap_create (const char *ifname)
{
  int if_qnum;
  struct nmreq nmr;
  char nm_ifname[IFNAMSIZ];

  struct nuse_vif_netmap *netmap = sim_malloc (sizeof (struct nuse_vif_netmap));
  sprintf(nm_ifname, "netmap:%s", ifname);
  netmap->nmd = nm_open (nm_ifname, NULL, NM_OPEN_IFNAME, NULL);
  if (netmap->nmd == NULL)
    {
      printf ("Unable to open %s: %s\n",
	      ifname, strerror (errno));
      sim_free (netmap);
      return NULL;
  }
  netmap->fd = netmap->nmd->fd;

  if (1) {
    struct netmap_if *nifp = netmap->nmd->nifp;
    struct nmreq *req = &netmap->nmd->req;
    int i;

    printf("nifp at offset %d, %d tx %d rx region %d\n",
	   req->nr_offset, req->nr_tx_rings, req->nr_rx_rings,
	   req->nr_arg2);
    for (i = 0; i <= req->nr_tx_rings; i++) {
      printf("   TX%d at 0x%lx\n", i,
	     (char *)NETMAP_TXRING(nifp, i) - (char *)nifp);
    }
    for (i = 0; i <= req->nr_rx_rings; i++) {
      printf ("   RX%d at 0x%lx\n", i,
	      (char *)NETMAP_RXRING(nifp, i) - (char *)nifp);
    }
  }


  struct nuse_vif *vif = sim_malloc (sizeof (struct nuse_vif));
  vif->type = NUSE_VIF_NETMAP;
  vif->private = netmap;
  return vif;
}

void
nuse_vif_netmap_delete (struct nuse_vif *vif)
{
  struct nuse_vif_netmap *netmap = vif->private;

  nm_close (netmap->nmd);
  close (netmap->fd);

  sim_free (netmap);
  sim_free (vif);
  return;
}

static struct nuse_vif_impl nuse_vif_netmap = {
  .read = nuse_vif_netmap_read,
  .write = nuse_vif_netmap_write,
  .create = nuse_vif_netmap_create,
  .delete = nuse_vif_netmap_delete,
};

extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

int nuse_vif_netmap_init (void)
{
  nuse_vif[NUSE_VIF_NETMAP] = &nuse_vif_netmap;
  return 0;
}
__define_initcall(nuse_vif_netmap_init, 1);
