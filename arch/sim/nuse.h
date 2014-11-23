#ifndef NUSE_H
#define NUSE_H

#include "nuse-vif.h"
struct pollfd;

/* nuse.c */
struct nuse_fd {
	int real_fd;
	struct epoll_fd *epoll_fd;
	struct socket *kern_sock;
};
extern struct nuse_fd nuse_fd_table[1024];

/* nuse-vif.c */
void *nuse_vif_create(enum viftype type, const char *ifname);
void nuse_vif_read(struct nuse_vif *vif, struct SimDevice *dev);
void nuse_vif_write(struct nuse_vif *vif, struct SimDevice *dev,
		    unsigned char *data, int len);
/* nuse-fiber.c */
void *nuse_fiber_new(void * (*callback)(void *),
		     void *context, uint32_t stackSize, const char *name);
void nuse_fiber_start(void *handler);
int nuse_fiber_isself(void *handler);
void nuse_fiber_wait(void *handler);
int nuse_fiber_wakeup(void *handler);
void nuse_fiber_free(void *handler);
void *nuse_fiber_new_from_caller(uint32_t stackSize, const char *name);
int nuse_fiber_is_stopped(void *handler);
void nuse_fiber_stop(void *handler);
void nuse_add_timer(unsigned long ns, void *(*func) (void *arg),
		    void *arg, void *handler);
void nuse_task_add(void *fiber);
void nuse_set_affinity(void);

/* nuse-poll.c */
int nuse_poll(struct pollfd *fds, unsigned int nfds,
	struct timespec *end_time);

#endif /* NUSE_H */
