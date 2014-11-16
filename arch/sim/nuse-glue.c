#include <net/net_namespace.h> 
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include "sim-init.h"
#include "sim-assert.h"
#include "nuse.h"
#include "sim.h"

//#include <sys/select.h>
typedef long int __fd_mask;
# define __FDS_BITS(set) ((set)->fds_bits)
# define __FD_ZERO(set)  \
  do {                                                                        \
    unsigned int __i;                                                         \
    fd_set *__arr = (set);                                                    \
    for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i)          \
      __FDS_BITS (__arr)[__i] = 0;                                            \
  } while (0)


#define __FD_SET(d, set) \
  ((void) (__FDS_BITS (set)[__FD_ELT (d)] |= __FD_MASK (d)))
#define __FD_CLR(d, set) \
  ((void) (__FDS_BITS (set)[__FD_ELT (d)] &= ~__FD_MASK (d)))
#define __FD_ISSET(d, set) \
  ((__FDS_BITS (set)[__FD_ELT (d)] & __FD_MASK (d)) != 0)

#define FD_SET(fd, fdsetp)      __FD_SET (fd, fdsetp)
#define FD_CLR(fd, fdsetp)      __FD_CLR (fd, fdsetp)
#define FD_ISSET(fd, fdsetp)    __FD_ISSET (fd, fdsetp)
#define FD_ZERO(fdsetp)         __FD_ZERO (fdsetp)
#define __NFDBITS       (8 * (int) sizeof (__fd_mask))
#define __FD_ELT(d)     ((d) / __NFDBITS)
#define __FD_MASK(d)    ((__fd_mask) 1 << ((d) % __NFDBITS))

// #include <sys/epoll.h> FIXME
/* Valid opcodes ( "op" parameter ) to issue to epoll_ctl().  */
#define EPOLL_CTL_ADD 1 /* Add a file descriptor to the interface.  */
#define EPOLL_CTL_DEL 2 /* Remove a file descriptor from the interface.  */
#define EPOLL_CTL_MOD 3 /* Change file descriptor epoll_event structure.  */

typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;      /* Epoll events */
  epoll_data_t data;    /* User data variable */
} __EPOLL_PACKED;

enum EPOLL_EVENTS
  {
    EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
    EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
    EPOLLOUT = 0x004,
#define EPOLLOUT EPOLLOUT
    EPOLLRDNORM = 0x040,
#define EPOLLRDNORM EPOLLRDNORM
    EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
    EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
    EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
    EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG
    EPOLLERR = 0x008,
#define EPOLLERR EPOLLERR
    EPOLLHUP = 0x010,
#define EPOLLHUP EPOLLHUP
    EPOLLRDHUP = 0x2000,
#define EPOLLRDHUP EPOLLRDHUP
    EPOLLWAKEUP = 1u << 29,
#define EPOLLWAKEUP EPOLLWAKEUP
    EPOLLONESHOT = 1u << 30,
#define EPOLLONESHOT EPOLLONESHOT
    EPOLLET = 1u << 31
#define EPOLLET EPOLLET
  };


extern int clock_gettime(clockid_t clk_id, struct timespec *tp);
extern void *malloc(size_t size);
extern void free(void *ptr);

extern void sim_update_jiffies (void);
extern void sim_softirq_wakeup (void);
extern ssize_t (*host_write) (int fd, const void *buf, size_t count);
extern ssize_t (*host_writev) (int fd, const struct iovec *iovec, size_t count);
extern ssize_t (*host_read)(int fd, void *buf, size_t count);
extern int (*host_ioctl)(int d, int request, ...);
extern int (*host_open) (const char *pathname, int flags, mode_t mode);
extern int (*host_open64) (const char *pathname, int flags, mode_t mode);
extern int (*host_close) (int fd);
extern int (*host__fxstat) (int version, int fd, void *buf);
extern int (*host__fxstat64) (int version, int fd, void *buf);
extern int nuse_poll (struct pollfd *fds, unsigned int nfds, struct timespec *end_time);
extern ssize_t (*host_pread64) (int fd, void *buf, size_t count, off_t offset);
extern ssize_t (*host_pwrite64) (int fd, const void *buf, size_t count, off_t offset);
extern int (*host_fcntl) (int fd, int cmd, ... /* arg */ );
extern int (*host_dup2) (int oldfd, int newfd);

__u64 curfd = 3;
struct nuse_fd nuse_fd_table[1024];

/* epoll relates */
struct epoll_fd
{
  struct epoll_fd *next;
  struct epoll_event *ev;
  int fd;
};


#define RETURN_void(rettype, v)			\
  ({						\
    (v);					\
    sim_softirq_wakeup ();			\
  })
#define RETURN_nvoid(rettype, v)		\
  ({						\
    rettype x = (v);				\
    sim_softirq_wakeup ();			\
    x;						\
  })

#define FORWARDER1(name,type,rettype,t0)		\
  extern rettype sim_sock_ ## name (t0);				\
  rettype name (t0 v0)		\
  {							\
    sim_update_jiffies ();				\
    return RETURN_##type (rettype, (sim_sock_ ## name (v0)));	\
  }
#define FORWARDER2(name,type,rettype,t0,t1)		\
  extern rettype sim_sock_ ## name (t0,t1);				\
  static rettype name (t0 v0, t1 v1)	\
  {							\
    sim_update_jiffies ();				\
    return RETURN_##type (rettype, (sim_sock_ ## name (v0, v1)));	\
  }
#define FORWARDER3(name,type,rettype,t0,t1,t2)				\
  extern rettype sim_sock_ ## name (t0,t1,t2);					\
  rettype name (t0 v0, t1 v1, t2 v2)		\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (sim_sock_ ## name (v0, v1, v2)));		\
  }
#define FORWARDER4(name,type,rettype,t0,t1,t2,t3)			\
  extern rettype sim_sock_ ## name (t0,t1,t2,t3);					\
  rettype name (t0 v0, t1 v1, t2 v2, t3 v3)	\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (sim_sock_ ## name (v0, v1, v2, v3)));		\
  }
#define FORWARDER5(name,type,rettype,t0,t1,t2,t3,t4)			\
  extern rettype sim_sock_ ## name (t0,t1,t2,t3,t4);					\
  static rettype name (t0 v0, t1 v1, t2 v2, t3 v3, t4 v4)	\
  {									\
    sim_update_jiffies ();						\
    return RETURN_##type (rettype, (sim_sock_ ## name (v0, v1, v2, v3, v4)));	\
  }


//FORWARDER3(socket, nvoid, int, int, int, int);
extern int sim_sock_socket (int,int,int, struct socket **);
int socket (int v0, int v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = sim_malloc (sizeof (struct socket));
  memset (kernel_socket, 0, sizeof (struct socket));
  int ret = sim_sock_socket (v0, v1, v2, &kernel_socket);
  if (ret < 0)
    {
      errno = -ret;
    }
  nuse_fd_table[curfd].kern_sock = kernel_socket;
  sim_softirq_wakeup ();
  return curfd++;
}
//FORWARDER1(close, nvoid, int, struct SimSocket *);
extern int sim_sock_close (struct socket *);
int close (int fd)
{
  if (!nuse_fd_table[fd].kern_sock)
    {
      if (nuse_fd_table[fd].epoll_fd > 0)
        {
          free (nuse_fd_table[fd].epoll_fd);
          nuse_fd_table[fd].epoll_fd = NULL;
          return 0;
        }
      else if (nuse_fd_table[fd].real_fd > 0)
        {
          return host_close (nuse_fd_table[fd].real_fd);
        }
      return EBADF;
    }

  sim_update_jiffies ();
  int ret = sim_sock_close (nuse_fd_table[fd].kern_sock);
  if (ret < 0)
    {
      errno = -ret;
    }
  nuse_fd_table[fd].kern_sock = 0;
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_recvmsg, nvoid, ssize_t, struct SimSocket *,struct msghdr *, int); */
extern ssize_t sim_sock_recvmsg (struct socket *, const struct msghdr *, int);
ssize_t recvmsg (int fd, const struct msghdr *v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  ssize_t ret = sim_sock_recvmsg (kernel_socket, v1, v2);
  if (ret < 0)
    {
      errno = -ret;
    }
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_sendmsg, nvoid, ssize_t, struct SimSocket *,const struct msghdr *, int); */
extern ssize_t sim_sock_sendmsg (struct socket *, const struct msghdr *, int);
ssize_t sendmsg (int fd, const struct msghdr *v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  ssize_t ret = sim_sock_sendmsg (kernel_socket, v1, v2);
  if (ret < 0) errno = -ret;
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_getsockname, nvoid, int, struct SimSocket *,struct sockaddr *, int *); */
extern int sim_sock_getsockname (struct socket *, struct sockaddr *name, int *namelen);
int getsockname (int fd, struct sockaddr *name, int *namelen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int ret = sim_sock_getsockname (kernel_socket, name, namelen);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_getpeername, nvoid, int, struct SimSocket *,struct sockaddr *, int *); */
extern int sim_sock_getpeername (struct socket *, struct sockaddr *name, int *namelen);
int getpeername (int fd, struct sockaddr *name, int *namelen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int ret = sim_sock_getsockname (kernel_socket, name, namelen);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_bind, nvoid, int, struct SimSocket *,const struct sockaddr *, int); */
extern int sim_sock_bind (struct socket *, const struct sockaddr *name, int namelen);
int bind (int fd, struct sockaddr *name, int namelen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int ret = sim_sock_bind (kernel_socket, name, namelen);
  sim_softirq_wakeup ();
  return ret;
}
//FORWARDER4(connect, nvoid, int, struct SimSocket *,const struct sockaddr *, int, int);
extern int sim_sock_connect (struct socket *, struct sockaddr *,int,int);
int connect (int fd, struct sockaddr *v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int ret = sim_sock_connect (kernel_socket, v1, v2, 0);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER2(sim_sock_listen,nvoid, int, struct SimSocket *,int); */
extern int sim_sock_listen (struct socket *socket, int backlog);
int listen (int fd, int v1)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int retval = sim_sock_listen (kernel_socket, v1);
  sim_softirq_wakeup ();
  return retval;
}
#if 0
/* FORWARDER2(sim_sock_shutdown,nvoid, int, struct SimSocket *,int); */
int sim_sock_shutdown (struct SimSocket *socket, int how)
{
  struct socket *sock = (struct socket *)socket;
  int retval = sock->ops->shutdown(sock, how);
  return retval;
}
#endif
/* FORWARDER3(sim_sock_accept,nvoid, int, struct SimSocket *,struct SimSocket **, int); */
extern int sim_sock_accept (struct socket *socket, struct socket **new_socket, int flags);
int accept4 (int fd, struct sockaddr *addr, int *addrlen, int flags)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  struct socket *new_socket = sim_malloc (sizeof (struct socket));
  int retval = sim_sock_accept (kernel_socket, &new_socket, flags);
  if (retval < 0)
    {
      errno = -retval;
      sim_free (new_socket);
      printf ("accept err\n");
      return -1;
    }
  if (addr != 0)
    {
      retval = sim_sock_getpeername (new_socket, addr, (int*)addrlen);
      if (retval < 0)
        {
          errno = -retval;
          sim_sock_close (new_socket);
          printf ("getpeername err\n");
          return -1;
        }
    }
  nuse_fd_table[curfd++].kern_sock = new_socket;
  sim_softirq_wakeup ();
  return curfd - 1;
}
int accept (int fd, struct sockaddr *addr, int *addrlen, int flags)
{
  return accept4 (fd, addr, addrlen, 0);
}

ssize_t write (int fd, const void *buf, size_t count)
{
  if (!nuse_fd_table[fd].kern_sock)
    {
      return host_write (nuse_fd_table[fd].real_fd, buf, count);
    }

  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = count;
  iov.iov_base = (void*)buf;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  return sendmsg (fd, &msg, 0);
}

ssize_t writev (int fd, const struct iovec *iov, size_t count)
{
  if (!nuse_fd_table[fd].kern_sock)
    {
      return host_writev (nuse_fd_table[fd].real_fd, iov, count);
    }

  struct msghdr msg;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = (struct iovec *)iov;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  return sendmsg (fd, &msg, 0);
}

ssize_t read (int fd, void *buf, size_t count)
{
  if (!nuse_fd_table[fd].kern_sock)
    {
      return host_read (nuse_fd_table[fd].real_fd, buf, count);
    }

  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = count;
  iov.iov_base = buf;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  ssize_t retval = recvmsg (fd, &msg, 0);
  return retval;
}

#ifdef _K_SS_MAXSIZE
#define SOCK_MAX_ADDRESS_SIZE _K_SS_MAXSIZE
#else
#define SOCK_MAX_ADDRESS_SIZE 128
#endif
ssize_t recvfrom (int fd, void *buf, size_t len, int flags,
                  struct sockaddr *from, int *fromlen)
{
  uint8_t address[SOCK_MAX_ADDRESS_SIZE];
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = len;
  iov.iov_base = buf;
  msg.msg_name = address;
  msg.msg_namelen = SOCK_MAX_ADDRESS_SIZE;
  ssize_t retval = recvmsg (fd, &msg, flags);
  if (retval != -1 && from != 0)
    {
      if (*fromlen < msg.msg_namelen)
        {
          errno = EINVAL;
          return -1;
        }
      else
        {
          *fromlen = msg.msg_namelen;
          memcpy (from, msg.msg_name, msg.msg_namelen);
        }
    }
  return retval;
}
int recv (int fd, void *buf, size_t count, int flags)
{
  return recvfrom (fd, buf, count, flags, 0, 0);
}

extern int sim_sock_setsockopt (struct socket *socket, int level, int optname,
                                const void *optval, int optlen);
int setsockopt(int fd, int level, int optname,
               const void *optval, int optlen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int retval = sim_sock_setsockopt (kernel_socket, level, optname, optval, optlen);
  if (retval < 0)
    {
      errno = -retval;
      sim_softirq_wakeup ();
      return -1;
    }
  sim_softirq_wakeup ();
  return retval;
}

extern int sim_sock_getsockopt (struct socket *socket, int level, int optname,
                                void *optval, int *optlen);
int getsockopt(int fd, int level, int optname,
               void *optval, int *optlen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = nuse_fd_table[fd].kern_sock;
  int retval = sim_sock_getsockopt (kernel_socket, level, optname, optval, optlen);
  if (retval < 0)
    {
      errno = -retval;
      sim_softirq_wakeup ();
      return -1;
    }
  sim_softirq_wakeup ();
  return retval;
}

int open (const char *pathname, int flags, mode_t mode)
{
  nuse_fd_table[curfd].real_fd = host_open (pathname, flags, mode);
  if (nuse_fd_table[curfd].real_fd < 0)
    {
      printf ("open error %d\n", errno);
      return -1;
    }
  return curfd++;
}

int open64 (const char *pathname, int flags, mode_t mode)
{
  nuse_fd_table[curfd].real_fd = host_open64 (pathname, flags, mode);
  //  printf ("%d, %llu %s %s\n", nuse_fd_table[curfd].real_fd, curfd, pathname, __FUNCTION__);
  if (nuse_fd_table[curfd].real_fd < 0)
    {
      printf ("open error %d\n", errno);
      return -1;
    }
  return curfd++;
}

int __fxstat (int ver, int fd, void *buf)
{
  if (nuse_fd_table[fd].real_fd < 0)
    {
      return -1;
    }
  return host__fxstat (ver, nuse_fd_table[fd].real_fd, buf);
}

int __fxstat64 (int ver, int fd, void *buf)
{
  if (nuse_fd_table[fd].real_fd < 0)
    {
      return -1;
    }
  return host__fxstat64 (ver, nuse_fd_table[fd].real_fd, buf);
}

ssize_t pread64 (int fd, void *buf, size_t count, off_t offset)
{
  if (nuse_fd_table[fd].real_fd < 0)
    {
      return -1;
    }
  return host_pread64 (nuse_fd_table[fd].real_fd, buf, count, offset);
}

ssize_t pwrite64 (int fd, const void *buf, size_t count, off_t offset)
{
  if (nuse_fd_table[fd].real_fd < 0)
    {
      return -1;
    }
  return host_pwrite64 (nuse_fd_table[fd].real_fd, buf, count, offset);
}

int fcntl (int fd, int cmd, ... /* arg */ )
{
  if (nuse_fd_table[fd].real_fd < 0)
    {
      return -1;
    }
  va_list vl;
  va_start (vl, cmd);
  unsigned long arg = va_arg (vl, unsigned long);
  va_end (vl);

  return host_fcntl (nuse_fd_table[fd].real_fd, cmd, arg);
}

int dup2 (int oldfd, int newfd)
{
  if (nuse_fd_table[oldfd].real_fd < 0)
    {
      return -1;
    }
  return host_dup2 (nuse_fd_table[oldfd].real_fd, 
                    nuse_fd_table[newfd].real_fd);
}

extern int sim_sock_ioctl (struct socket *socket, int request, char *argp);
int ioctl (int fd, int request, ...)
{
  va_list vl;
  va_start (vl, request);
  char *argp = va_arg (vl, char*);
  va_end (vl);

  if (!nuse_fd_table[fd].kern_sock)
    {
      return host_fcntl (nuse_fd_table[fd].real_fd, request, argp);
    }

  return sim_sock_ioctl (nuse_fd_table[fd].kern_sock, request, argp);
}


int
poll (struct pollfd *fds, nfds_t nfds, int timeout)
{
  struct timespec *to = NULL, end_time;
  sim_update_jiffies ();

  if (timeout >= 0) {
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    end_time.tv_sec += timeout / MSEC_PER_SEC;
    end_time.tv_nsec += NSEC_PER_MSEC * (timeout % MSEC_PER_SEC);
    to = &end_time;
  }

  return nuse_poll (fds, nfds, to);
}

int
select (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
        struct timeval *timeout)
{
  struct pollfd pollFd[nfds];
  int fd;

  memset (pollFd, 0, sizeof (struct pollfd) * nfds);

  if (nfds == -1)
    {
      errno = EINVAL;
      return -1;
    }
  if (readfds == 0 && writefds == 0 && exceptfds == 0)
    {
      errno = EINVAL;
      return -1;
    }
  if (timeout)
    {
      if (timeout->tv_sec < 0 || timeout->tv_usec < 0)
        {
          errno = EINVAL;
          return -1;
        }
    }

  for (fd = 0; fd < nfds; fd++)
    {
      int event = 0;

      if (readfds != 0 && FD_ISSET (fd, readfds))
        {
          event |= POLLIN;
        }
      if (writefds != 0 &&  FD_ISSET (fd, writefds))
        {
          event |= POLLOUT;
        }
      if (exceptfds != 0 && FD_ISSET (fd, exceptfds))
        {
          event |= POLLPRI;
        }

      pollFd[fd].events = event;
      pollFd[fd].revents = 0;
      pollFd[fd].fd = fd;

      if (event)
        {
          if (!nuse_fd_table[fd].kern_sock)
            {
              errno = EBADF;
              return -1;
            }
        }
    }

  // select(2):
  // Some  code  calls  select() with all three sets empty, nfds zero, and a
  // non-NULL timeout as a fairly portable way to sleep with subsecond
  // precision.
  // 130825: this condition will be passed by dce_poll ()

  int to_msec = -1;

  if (timeout)
    {
      to_msec = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }

  int ret = poll (pollFd, nfds, to_msec);

  if (readfds)
    {
      FD_ZERO (readfds);
    }
  if (writefds)
    {
      FD_ZERO (writefds);
    }
  if (exceptfds)
    {
      FD_ZERO (exceptfds);
    }
    
  if (ret > 0)
    {
      ret = 0;
      for (fd = 0; fd < nfds; fd++)
        {
          if (readfds && ((POLLIN & pollFd[fd].revents) || (POLLHUP & pollFd[fd].revents)
                          || (POLLERR & pollFd[fd].revents)))
            {
              FD_SET (pollFd[fd].fd, readfds);
              ret++;
            }
          if (writefds && (POLLOUT & pollFd[fd].revents))
            {
              FD_SET (pollFd[fd].fd, writefds);
              ret++;
            }
          if (exceptfds && (POLLPRI & pollFd[fd].revents))
            {
              FD_SET (pollFd[fd].fd, exceptfds);
              ret++;
            }
        }
    }
  return ret;
}

int
epoll_create (int size)
{
  struct epoll_fd *epfd = malloc (sizeof (struct epoll_fd));
  memset (epfd, 0, sizeof (struct epoll_fd));
  nuse_fd_table[curfd].epoll_fd = epfd;
  return curfd++;
}

int
epoll_ctl (int epollfd, int op, int fd, struct epoll_event *event)
{
  struct epoll_fd *prev, *epfd = nuse_fd_table[epollfd].epoll_fd;
  if (!epfd)
    return EBADF;

  struct epoll_event *ev;

  switch (op)
    {
    case EPOLL_CTL_ADD:
      ev = (struct epoll_event *)malloc (sizeof (struct epoll_event));
      memset (ev, 0, sizeof (struct epoll_event));
      memcpy (ev, event, sizeof (struct epoll_event));

      if (!epfd->ev)
        {
          epfd->ev = ev;
          epfd->fd = fd;
        }
      else
        {
          prev = epfd;
          while (epfd->next)
            {
              prev = epfd;
              epfd = epfd->next;
            }

          epfd->next = malloc (sizeof (struct epoll_fd));
          memset (epfd->next, 0, sizeof (struct epoll_fd));
          epfd->next->ev = ev;
          epfd->next->fd = fd;
        }
      break;
    case EPOLL_CTL_MOD:
      while (epfd && epfd->fd != fd)
        {
          epfd = epfd->next;
        }
      ev = epfd->ev;
      memcpy (ev, event, sizeof (struct epoll_event));
      epfd->fd = fd;
      break;
    case EPOLL_CTL_DEL:
      while (epfd && epfd->fd != fd)
        {
          prev = epfd;
          epfd = epfd->next;
        }
      prev->next = epfd->next;
      ev = epfd->ev;
      free (ev);
      free (epfd);
      break;
    default:
      break;
    }

  return 0;
}

int
epoll_wait (int epollfd, struct epoll_event *events,
           int maxevents, int timeout)
{
  struct epoll_fd *cur, *epfd = nuse_fd_table[epollfd].epoll_fd;
  if (!epfd)
    return EBADF;

  struct epoll_event *ev;

  struct pollfd pollFd[maxevents];
  memset (pollFd, 0, sizeof (struct pollfd) * maxevents);
  int j = 0;

  for (cur = epfd; cur && cur->ev ; cur = cur->next)
    {
      struct epoll_event *ev = cur->ev;
      int pevent = 0;
      if (ev->events & EPOLLIN)
        {
          pevent |= POLLIN;
        }
      if (ev->events & EPOLLOUT)
        {
          pevent |= POLLOUT;
        }

      pollFd[j].events = pevent;
      pollFd[j].fd = cur->fd;
      pollFd[j++].revents = 0;
    }

  int pollRet = poll (pollFd, maxevents, timeout);
  if (pollRet > 0)
    {
      pollRet = 0;
      /* FIXME: c10k... far fast */
      for (j = 0; j < maxevents; j++)
        {
          int fd = pollFd[j].fd;
          struct epoll_event *rev;
          for (cur = epfd; cur && epfd->ev ; cur = cur->next)
            {
              rev = cur->ev;
              if (cur->fd == fd)
                break;
            }

          if ((POLLIN & pollFd[j].revents) || (POLLHUP & pollFd[j].revents)
              || (POLLERR & pollFd[j].revents))
            {
              memcpy (events, rev, sizeof (struct epoll_event));
              events->events = pollFd[j].revents;
              printf ("epoll woke up for read with %d\n", fd);
              pollRet++;
              events++;
            }
          if (POLLOUT & pollFd[j].revents)
            {
              memcpy (events, rev, sizeof (struct epoll_event));
              events->events = pollFd[j].revents;
              // *events = *epollFd->evs[fd];
              // events->data.fd = fd;
              printf ("epoll woke up for write with %d\n", fd);
              pollRet++;
              events++;
            }
          if (POLLPRI & pollFd[j].revents)
            {
              memcpy (events, rev, sizeof (struct epoll_event));
              events->events = pollFd[j].revents;
              printf ("epoll woke up for other with %d\n", fd);
              // *events = *epollFd->evs[fd];
              // events->data.fd = fd;
              pollRet++;
              events++;
            }

        }
    }

  return pollRet;
}
       
