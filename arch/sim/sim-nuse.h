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
};

struct nuse_vif_impl {
  void (*read) (struct nuse_vif *, struct SimDevice *);
  void (*write) (struct nuse_vif *, struct SimDevice *, 
                 unsigned char *, int);
  void *(*create) (void);
  void (*delete) (struct nuse_vif *);
};

/* FIXME */
struct SimTask
{
  struct list_head head;
  struct task_struct kernel_task;
  void *private;
};

/* sim-nuse-vif.c */
extern void * nuse_vif_create (enum viftype);
extern void nuse_vif_read (struct nuse_vif *vif, struct SimDevice *dev);
extern void nuse_vif_write (struct nuse_vif *vif, struct SimDevice *dev, 
                            unsigned char *data, int len);
/* sim-fiber.c */
extern void *sim_fiber_new (void* (*callback)(void *),
                            void *context, uint32_t stackSize, const char *name);
extern void sim_fiber_start (void * handler);
extern int sim_fiber_isself (void *handler);
extern void sim_fiber_wait (void *handler);
extern int sim_fiber_wakeup (void *handler);
extern void sim_fiber_free (void * handler);
extern void *sim_fiber_new_from_caller (uint32_t stackSize, const char *name);
extern int sim_fiber_is_stopped (void * handler);
extern void sim_fiber_stop (void * handler);
