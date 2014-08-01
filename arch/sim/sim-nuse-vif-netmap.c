#include <net/sock.h>
#include "sim-init.h"
#include "sim-assert.h"
#include "sim-nuse.h"
#include "sim.h"


void
nuse_vif_netmap_read (struct nuse_vif *vif, struct SimDevice *dev)
{
  return;
}

void
nuse_vif_netmap_write (struct nuse_vif *vif, struct SimDevice *dev, 
                unsigned char *data, int len)
{
  return;
}

void *
nuse_vif_netmap_create (void)
{
  struct nuse_vif *vif = sim_malloc (sizeof (struct nuse_vif));
  vif->type = NUSE_VIF_NETMAP;
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
