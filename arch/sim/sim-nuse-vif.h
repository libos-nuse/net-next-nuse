struct SimDevice;

enum viftype
  {
    NUSE_VIF_RAWSOCK = 0,
    NUSE_VIF_NETMAP,            /* not yet */
    NUSE_VIF_DPDK,               /* not yet */
    NUSE_VIF_MAX
  };

struct nuse_vif
{
  int sock;
  enum viftype type;
  void *private;
};

struct nuse_vif_impl {
  void (*read) (struct nuse_vif *, struct SimDevice *);
  void (*write) (struct nuse_vif *, struct SimDevice *, 
                 unsigned char *, int);
  void *(*create) (const char *);
  void (*delete) (struct nuse_vif *);
};
