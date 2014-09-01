#ifndef NUSE_HOSTCALLS_H
#define NUSE_HOSTCALLS_H

//#include <sys/socket.h>
#include <poll.h>

/* nuse-hostcalls.c */
extern int (*host_socket) (int fd, int type, int proto);
extern int (*host_close) (int fd);
extern int (*host_bind)(int, const struct sockaddr *, int);
extern ssize_t (*host_write) (int fd, const void *buf, size_t count);
extern ssize_t (*host_writev) (int fd, const struct iovec *iovec, size_t count);
extern int (*host_open) (const char *pathname, int flags);
extern int (*host_ioctl)(int d, int request, ...);
extern int (*host_poll)(struct pollfd *, int, int);
extern int (*host_pthread_create)(pthread_t *, const pthread_attr_t *,
                                  void *(*) (void *), void *);

#endif /* NUSE_HOSTCALLS_H */
