#ifndef NUSE_H
#define NUSE_H

#include "nuse-vif.h"

/* nuse-vif.c */
extern void *nuse_vif_create (enum viftype type, const char *ifname);
extern void nuse_vif_read (struct nuse_vif *vif, struct SimDevice *dev);
extern void nuse_vif_write (struct nuse_vif *vif, struct SimDevice *dev, 
                            unsigned char *data, int len);
/* nuse-fiber.c */
extern void *nuse_fiber_new (void* (*callback)(void *),
                            void *context, uint32_t stackSize, const char *name);
extern void nuse_fiber_start (void * handler);
extern int nuse_fiber_isself (void *handler);
extern void nuse_fiber_wait (void *handler);
extern int nuse_fiber_wakeup (void *handler);
extern void nuse_fiber_free (void * handler);
extern void *nuse_fiber_new_from_caller (uint32_t stackSize, const char *name);
extern int nuse_fiber_is_stopped (void * handler);
extern void nuse_fiber_stop (void * handler);

#endif /* NUSE_H */
