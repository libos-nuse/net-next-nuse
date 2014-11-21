#ifndef NUSE_HOSTCALLS_H
#define NUSE_HOSTCALLS_H

struct pollfd;
struct pthread;
struct pthread_attr;

/* nuse-hostcalls.c */
void nuse_hijack_init(void);

extern int (*host_socket)(int fd, int type, int proto);
extern int (*host_close)(int fd);
extern int (*host_bind)(int, const struct sockaddr *, int);
extern ssize_t (*host_read)(int fd, void *buf, size_t count);
extern ssize_t (*host_write)(int fd, const void *buf, size_t count);
extern ssize_t (*host_writev)(int fd, const struct iovec *iovec, size_t count);
extern int (*host_open)(const char *pathname, int flags, mode_t mode);
extern int (*host_open64)(const char *pathname, int flags, mode_t mode);
extern int (*host_ioctl)(int d, int request, ...);
extern int (*host_poll)(struct pollfd *, int, int);
extern int (*host_pthread_create)(struct pthread *, const struct pthread_attr *,
				  void *(*)(void *), void *);

#endif /* NUSE_HOSTCALLS_H */
