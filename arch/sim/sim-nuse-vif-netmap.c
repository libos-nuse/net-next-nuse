#include <net/sock.h>
#include <linux/netdevice.h>
#define _GNU_SOURCE /* Get RTLD_NEXT */
#include <dlfcn.h>

#include <sys/mman.h>
#include <net/netmap.h>
//#include <net/netmap_user.h>

#include "sim-init.h"
#include "sim-assert.h"
#include "sim-nuse.h"
#include "sim.h"

/* helper macro */
#define _NETMAP_OFFSET(type, ptr, offset) \
  ((type)(void *)((char *)(ptr) + (offset)))

#define NETMAP_IF(_base, _ofs)  _NETMAP_OFFSET(struct netmap_if *, _base, _ofs)

#define NETMAP_TXRING(nifp, index) _NETMAP_OFFSET(struct netmap_ring *, \
						  nifp, (nifp)->ring_ofs[index] )

#define NETMAP_RXRING(nifp, index) _NETMAP_OFFSET(struct netmap_ring *, \
						  nifp, (nifp)->ring_ofs[index + (nifp)->ni_tx_rings + 1] )

#define NETMAP_BUF(ring, index)                         \
  ((char *)(ring) + (ring)->buf_ofs + ((index)*(ring)->nr_buf_size))

#define NETMAP_BUF_IDX(ring, buf)                       \
  ( ((char *)(buf) - ((char *)(ring) + (ring)->buf_ofs) ) / \
    (ring)->nr_buf_size )

static inline uint32_t
nm_ring_space(struct netmap_ring *ring)
{
  int ret = ring->tail - ring->cur;
  if (ret < 0)
    ret += ring->num_slots;
  return ret;
}

static inline uint32_t
nm_ring_next(struct netmap_ring *r, uint32_t i)
{
  return ( unlikely(i + 1 == r->num_slots) ? 0 : i + 1);
}

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))


struct nuse_vif_netmap
{
  int tx_fd;
  int rx_fd;
  struct netmap_ring *tx_ring;
  struct netmap_ring *rx_ring;
};

extern struct SimDevicePacket sim_dev_create_packet (struct SimDevice *dev, int size);
extern void sim_dev_rx (struct SimDevice *device, struct SimDevicePacket packet);
extern void *sim_dev_get_private (struct SimDevice *);
extern void sim_softirq_wakeup (void);
extern void *sim_malloc (unsigned long size);
static int (*host_poll)(struct pollfd *, int, int) = NULL;

void
nuse_vif_netmap_read (struct nuse_vif *vif, struct SimDevice *dev)
{
  struct nuse_vif_netmap *netmap = vif->private;
  struct netmap_ring *ring = netmap->rx_ring;
  char buf[8192];

  struct pollfd x[1];
  x[0].fd = netmap->rx_fd;
  x[0].events = POLLIN;

  if (!host_poll)
    {
      host_poll = dlsym (RTLD_NEXT, "poll");
      if (!host_poll)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }

  while (1)
    {
      if (host_poll (x, 1, 0) == 0)
        continue;

      {
        uint32_t i;
        uint32_t idx;
        bool morefrag;
	int size;

        do {
          i = ring->cur;
	  idx = ring->slot[i].buf_idx;
	  morefrag = (ring->slot[i].flags & NS_MOREFRAG);

	  size = ring->slot[i].len;
	  ring->cur = ring->head = nm_ring_next(ring, i);

	  if (unlikely (nm_ring_empty (ring) && morefrag)) 
	    {
	      printf ("[netmap_send] ran out of slots, with a pending"
		      "incomplete packet\n");
	    }

	  struct ethhdr
	  {
	    unsigned char   h_dest[6];
	    unsigned char   h_source[6];
	    uint16_t        h_proto;
	  } *hdr = (struct ethhdr *)(u_char *)NETMAP_BUF(ring, idx);

	  /* check dest mac for promisc */
	  if (memcmp (hdr->h_dest, ((struct net_device *)dev)->dev_addr, 6) &&
	      memcmp (hdr->h_dest, ((struct net_device *)dev)->broadcast, 6))
	    {
	      sim_softirq_wakeup ();
	      continue;
	    }

	  /* printf ("proto = 0x%x, dst= %X:%X:%X:%X:%X:%X\n",  */
	  /* 	  hdr->h_proto, hdr->h_dest[0], hdr->h_dest[1], hdr->h_dest[2], */
	  /* 	  hdr->h_dest[3], hdr->h_dest[4],hdr->h_dest[5]); */

	  struct SimDevicePacket packet = sim_dev_create_packet (dev, size);
	  /* XXX: FIXME should not copy */
	  memcpy (packet.buffer, hdr, size);

	  sim_dev_rx (dev, packet);
	  sim_softirq_wakeup ();
        } while (!nm_ring_empty(ring) && morefrag);

      }
    }
  return;
}

void
nuse_vif_netmap_write (struct nuse_vif *vif, struct SimDevice *dev, 
                unsigned char *data, int len)
{
  struct nuse_vif_netmap *netmap = vif->private;
  struct netmap_ring *ring = netmap->tx_ring;

  uint32_t last;
  uint32_t idx;
  uint8_t *dst;
  int j;
  uint32_t i;

  if (unlikely(!ring)) {
    /* Drop the packet. */
    return;
  }

  struct pollfd x[1];
  x[0].fd = netmap->tx_fd;
  x[0].events = POLLOUT;

  if (!host_poll)
    {
      host_poll = dlsym (RTLD_NEXT, "poll");
      if (!host_poll)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
	  sim_assert (0);
        }
    }

  while (1)
    {
      if (host_poll (x, 1, 0) == 0)
	{
	  perror ("poll");
	  continue;
	}
      break;
    }

  last = i = ring->cur;

  for (j = 0; j < 1; j++) {
    int iov_frag_size = len;
    int offset = 0;
    int nm_frag_size;

    /* Split each iovec fragment over more netmap slots, if
       necessary. */
    while (iov_frag_size) {
      nm_frag_size = MIN(iov_frag_size, ring->nr_buf_size);

      if (unlikely(nm_ring_empty(ring))) {
	/* We run out of netmap slots while splitting the
	   iovec fragments. */
	//	netmap_write_poll(s, true);
	return;
      }

      idx = ring->slot[i].buf_idx;
      dst = (uint8_t *)NETMAP_BUF(ring, idx);

      ring->slot[i].len = nm_frag_size;
      ring->slot[i].flags = NS_MOREFRAG;
      memcpy (dst, data + offset, nm_frag_size);

      last = i;
      i = nm_ring_next(ring, i);

      offset += nm_frag_size;
      iov_frag_size -= nm_frag_size;
    }
  }
  /* The last slot must not have NS_MOREFRAG set. */
  ring->slot[last].flags &= ~NS_MOREFRAG;

  /* Now update ring->cur and ring->head. */
  ring->cur = ring->head = i;

  ioctl(netmap->tx_fd, NIOCTXSYNC, NULL);

  return;
}

static int
extract_netmap_ring (const char * ifname, int q, struct netmap_ring ** ring,
                     int x, int w)
{
  int fd;
  char * mem;
  struct nmreq nmr;
  struct netmap_if * nifp;

  /* open netmap for  ring */
  fd = open ("/dev/netmap", O_RDWR);
  if (fd < 0) {
    printf ("unable to open /dev/netmap");
    return -1;
  }

  memset (&nmr, 0, sizeof (nmr));
  strcpy (nmr.nr_name, ifname);
  nmr.nr_version = NETMAP_API;
  nmr.nr_ringid = q | (NETMAP_NO_TX_POLL | NETMAP_DO_RX_POLL);
  nmr.nr_flags = w;

  if (ioctl (fd, NIOCREGIF, &nmr) < 0) {
    printf ("unable to register interface %s", ifname);
    return -1;
  }

  mem = mmap (NULL, nmr.nr_memsize,
	      PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (mem == MAP_FAILED) {
    printf ("unable to mmap");
    return -1;
  }

  nifp = NETMAP_IF (mem, nmr.nr_offset);

  if (x > 0)
    *ring = NETMAP_TXRING (nifp, q);
  else
    *ring = NETMAP_RXRING (nifp, q);

  return fd;
}
#define extract_netmap_hw_tx_ring(i, q, r) \
  extract_netmap_ring (i, q, r, 1, NR_REG_ONE_NIC)
#define extract_netmap_hw_rx_ring(i, q, r) \
  extract_netmap_ring (i, q, r, 0, NR_REG_ONE_NIC)
#define extract_netmap_sw_tx_ring(i, q, r) \
  extract_netmap_ring (i, q, r, 1, NR_REG_SW)
#define extract_netmap_sw_rx_ring(i, q, r) \
  extract_netmap_ring (i, q, r, 0, NR_REG_SW)


void *
nuse_vif_netmap_create (const char *ifname)
{
  int if_qnum, idx = 0;
  struct nmreq nmr;

  struct nuse_vif_netmap *netmap = sim_malloc (sizeof (struct nuse_vif_netmap));
#if 0
  netmap->fd = open ("/dev/netmap", O_RDWR);
  if (netmap->fd < 0) 
    {
      perror ("/dev/nemap open");
      return NULL;
    }

  memset (&nmr, 0, sizeof (nmr));
  nmr.nr_version = NETMAP_API;
  strncpy (nmr.nr_name, ifname, IFNAMSIZ - 1);
  if (ioctl (netmap->fd, NIOCGINFO, &nmr) < 0)
    {
      printf ("unable to get interface info for %s", ifname);
      perror ("ioctl");
      return NULL;
    }
  if_qnum = nmr.nr_rx_rings;
#endif

  netmap->rx_fd = extract_netmap_hw_rx_ring (ifname, idx, &netmap->rx_ring);
  netmap->tx_fd = extract_netmap_hw_tx_ring (ifname, idx, &netmap->tx_ring);

  struct nuse_vif *vif = sim_malloc (sizeof (struct nuse_vif));
  vif->type = NUSE_VIF_NETMAP;
  vif->private = netmap;
  return vif;
}

void
nuse_vif_netmap_delete (struct nuse_vif *vif)
{
  return;
}

static struct nuse_vif_impl nuse_vif_netmap = {
  .read = nuse_vif_netmap_read,
  .write = nuse_vif_netmap_write,
  .create = nuse_vif_netmap_create,
  .delete = nuse_vif_netmap_delete,
};

extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

static int __init nuse_vif_netmap_init (void)
{
  nuse_vif[NUSE_VIF_NETMAP] = &nuse_vif_netmap;
  return 0;
}
core_initcall(nuse_vif_netmap_init);
