#include <net/net_namespace.h> 
#include <errno.h>
#include <poll.h>
#include "sim-init.h"
#include "sim-assert.h"
#include "sim-nuse.h"
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

extern int clock_gettime(clockid_t clk_id, struct timespec *tp);

extern void sim_update_jiffies (void);
extern void sim_softirq_wakeup (void);

__u64 curfd = 3;
struct socket *g_fd_table[1024];

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
  g_fd_table [curfd++] = kernel_socket;
  sim_softirq_wakeup ();
  return curfd - 1;
}
//FORWARDER1(close, nvoid, int, struct SimSocket *);
extern int sim_sock_close (struct socket *);
int close (int fd)
{
  sim_update_jiffies ();
  int ret = sim_sock_close (g_fd_table[fd]);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_recvmsg, nvoid, ssize_t, struct SimSocket *,struct msghdr *, int); */
extern ssize_t sim_sock_recvmsg (struct socket *, const struct msghdr *, int);
ssize_t recvmsg (int fd, const struct msghdr *v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[fd];
  ssize_t ret = sim_sock_recvmsg (kernel_socket, v1, v2);
  if (ret < 0) errno = -ret;
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_sendmsg, nvoid, ssize_t, struct SimSocket *,const struct msghdr *, int); */
extern ssize_t sim_sock_sendmsg (struct socket *, const struct msghdr *, int);
ssize_t sendmsg (int fd, const struct msghdr *v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[fd];
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
  struct socket *kernel_socket = g_fd_table[fd];
  int ret = sim_sock_getsockname (kernel_socket, name, namelen);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_getpeername, nvoid, int, struct SimSocket *,struct sockaddr *, int *); */
extern int sim_sock_getpeername (struct socket *, struct sockaddr *name, int *namelen);
int getpeername (int fd, struct sockaddr *name, int *namelen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[fd];
  int ret = sim_sock_getsockname (kernel_socket, name, namelen);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER3(sim_sock_bind, nvoid, int, struct SimSocket *,const struct sockaddr *, int); */
extern int sim_sock_bind (struct socket *, const struct sockaddr *name, int namelen);
int bind (int fd, struct sockaddr *name, int namelen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[fd];
  int ret = sim_sock_bind (kernel_socket, name, namelen);
  sim_softirq_wakeup ();
  return ret;
}
//FORWARDER4(connect, nvoid, int, struct SimSocket *,const struct sockaddr *, int, int);
extern int sim_sock_connect (struct socket *, struct sockaddr *,int,int);
int connect (int v0, struct sockaddr *v1, int v2)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[v0];
  int ret = sim_sock_connect (kernel_socket, v1, v2, 0);
  sim_softirq_wakeup ();
  return ret;
}
/* FORWARDER2(sim_sock_listen,nvoid, int, struct SimSocket *,int); */
extern int sim_sock_listen (struct socket *socket, int backlog);
int listen (int v0, int v1)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[v0];
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
int accept (int fd, struct sockaddr *addr, int *addrlen)
{
  int flags = 0;                /* FIXME: XXX */
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[fd];
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
  g_fd_table [curfd++] = new_socket;
  sim_softirq_wakeup ();
  return curfd - 1;
}

ssize_t write (int fd, const void *buf, size_t count)
{
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

ssize_t read (int fd, void *buf, size_t count)
{
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
int setsockopt(int sockfd, int level, int optname,
               const void *optval, int optlen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[sockfd];
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
int getsockopt(int sockfd, int level, int optname,
               void *optval, int *optlen)
{
  sim_update_jiffies ();
  struct socket *kernel_socket = g_fd_table[sockfd];
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


void sim_poll_event (int flag, void *context)
{
  sim_assert (1);
  return;
}

/* FIXME: dup */
struct poll_table_ref {
  int ret;
  void *opaque;
};
extern void sim_sock_poll (struct socket *socket, struct poll_table_ref *ret);
int
poll (struct pollfd *fds, nfds_t nfds, int timeout)
{
  sim_update_jiffies ();

  int count = 0;
  int timed_out = 0;
  int i;

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  if (0 == timeout)
    {
      timed_out = 1;
    }

  for (i = 0; i < nfds; ++i)
    {
      // initialize all outgoing events.
      fds[i].revents = 0;
    }

  /* input information */
  struct poll_table_ref poll_table;
  poll_table.opaque = &poll_table;
  poll_table.ret = fds[i].events | POLLERR | POLLHUP;

restart:

  /* call (sim) kernel side */
  for (i = 0; i < nfds; ++i)
    {
      if (!g_fd_table[i])
        {
          continue;
        }

      sim_sock_poll (g_fd_table[i], &poll_table);
      int mask = poll_table.ret;
      mask &= (fds[i].events | POLLERR | POLLHUP);
      fds[i].revents = mask;
      if (mask)
        {
          count++;
        }
    }

  if (count || timeout == 0)
    {
      return count;
    }

  if (timeout < 0)
    {
      /* XXX */
      goto restart;
    }
  else
    {
      clock_gettime(CLOCK_MONOTONIC, &end);
      if (((end.tv_sec - start.tv_sec) * 1000 +
           (end.tv_nsec - start.tv_nsec) / 1000000) > timeout)
        {
          return count;
        }
      else
        {
          goto restart;
        }
    }

  return count;
}

int
select (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
        struct timeval *timeout)
{
  struct pollfd pollFd[nfds];
  int fd;

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
          if (!g_fd_table[fd])
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
