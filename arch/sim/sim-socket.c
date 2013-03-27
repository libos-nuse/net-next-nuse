#include "sim-init.h"
#include "sim.h"
#include <linux/net.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <net/sock.h>
#include <net/tcp_states.h>
#include <net/inet_connection_sock.h>

struct SimSocket
{};

static struct iovec *copy_iovec (const struct iovec *input, int len)
{
  int size = sizeof (struct iovec) * len;
  struct iovec *output = sim_malloc (size);
  if (!output)
    return NULL;
  sim_memcpy (output, input, size);
  return output;
}

int sim_sock_socket (int domain, int type, int protocol, struct SimSocket **socket)
{
  struct socket **kernel_socket = (struct socket **)socket;
  int retval = sock_create (domain, type, protocol, kernel_socket);
  return retval;
}
int sim_sock_close (struct SimSocket *socket)
{
  struct socket *kernel_socket = (struct socket *)socket;
  sock_release (kernel_socket);
  return 0;
}
static size_t iov_size(const struct msghdr *msg)
{
  size_t i;
  size_t size = 0;
  for (i = 0; i < msg->msg_iovlen; i++)
    {
      size += msg->msg_iov[i].iov_len;
    }
  return size;
}
ssize_t sim_sock_recvmsg (struct SimSocket *socket, struct msghdr *msg, int flags)
{
  struct socket *kernel_socket = (struct socket *)socket;
  struct iovec *kernel_iov = copy_iovec (msg->msg_iov, msg->msg_iovlen);
  struct iovec *user_iov = msg->msg_iov;
  struct cmsghdr *user_cmsgh = msg->msg_control;
  size_t user_cmsghlen = msg->msg_controllen;
  msg->msg_iov = kernel_iov;
  int retval = sock_recvmsg (kernel_socket, msg, iov_size (msg), flags);
  msg->msg_iov = user_iov;
  msg->msg_control = user_cmsgh;
  msg->msg_controllen = user_cmsghlen - msg->msg_controllen;
  sim_free (kernel_iov);
  return retval;
}
ssize_t sim_sock_sendmsg (struct SimSocket *socket, const struct msghdr *msg, int flags)
{
  struct socket *kernel_socket = (struct socket *)socket;
  struct iovec *kernel_iov = copy_iovec (msg->msg_iov, msg->msg_iovlen);
  struct msghdr kernel_msg = *msg;
  kernel_msg.msg_flags = flags;
  kernel_msg.msg_iov = kernel_iov;
  int retval = sock_sendmsg (kernel_socket, &kernel_msg, iov_size (msg));
  sim_free (kernel_iov);
  return retval;
}
int sim_sock_getsockname (struct SimSocket *socket, struct sockaddr *name, int *namelen)
{
  struct socket *sock = (struct socket *)socket;
  int retval = sock->ops->getname(sock, name, namelen, 0);
  return retval;
}
int sim_sock_getpeername (struct SimSocket *socket, struct sockaddr *name, int *namelen)
{
  struct socket *sock = (struct socket *)socket;
  int retval = sock->ops->getname(sock, name, namelen, 1);
  return retval;
}
int sim_sock_bind (struct SimSocket *socket, const struct sockaddr *name, int namelen)
{
  struct socket *sock = (struct socket *)socket;
  struct sockaddr_storage address;
  memcpy (&address, name, namelen);
  int retval = sock->ops->bind(sock, (struct sockaddr *)&address, namelen);
  return retval;
}
int sim_sock_connect (struct SimSocket *socket, const struct sockaddr *name, int namelen, int flags)
{
  struct socket *sock = (struct socket *)socket;
  struct sockaddr_storage address;
  memcpy (&address, name, namelen);
  int retval = sock->ops->connect(sock, (struct sockaddr *)&address, namelen, flags);
  return retval;
}
int sim_sock_listen (struct SimSocket *socket, int backlog)
{
  struct socket *sock = (struct socket *)socket;
  int retval = sock->ops->listen(sock, backlog);
  return retval;
}
int sim_sock_shutdown (struct SimSocket *socket, int how)
{
  struct socket *sock = (struct socket *)socket;
  int retval = sock->ops->shutdown(sock, how);
  return retval;
}
int sim_sock_accept (struct SimSocket *socket, struct SimSocket **new_socket, int flags)
{
  struct socket *sock, *newsock;
  int err;
  sock = (struct socket *)socket;

  // the fields do not matter here. If we could, 
  // we would call sock_alloc but it's not exported.
  err = sock_create_lite (0, 0, 0, &newsock);
  if (err < 0)
    {
      return err;
    }
  newsock->type = sock->type;
  newsock->ops = sock->ops;

  err = sock->ops->accept(sock, newsock, flags);
  if (err < 0)
    {
      sock_release (newsock);
      return err;
    }
  *new_socket = (struct SimSocket *)newsock;
  return 0;
}
int sim_sock_ioctl (struct SimSocket *socket, int request, char *argp)
{
  struct socket *sock = (struct socket *)socket;
  struct sock *sk;
  struct net *net;
  int err;

  sk = sock->sk;
  net = sock_net(sk);

  err = sock->ops->ioctl(sock, request, (long)argp);

  /*
   * If this ioctl is unknown try to hand it down
   * to the NIC driver.
   */
  if (err == -ENOIOCTLCMD)
    err = dev_ioctl(net, request, argp);
  return err;
}
int sim_sock_setsockopt (struct SimSocket *socket, int level, int optname,
			 const void *optval, int optlen)
{
  struct socket *sock = (struct socket *)socket;
  char *coptval = (char *)optval;
  int err;
  if (level == SOL_SOCKET)
    err = sock_setsockopt(sock, level, optname, coptval, optlen);
  else
    err = sock->ops->setsockopt(sock, level, optname, coptval, optlen);
  return err;
}
int sim_sock_getsockopt (struct SimSocket *socket, int level, int optname,
			 void *optval, int *optlen)
{
  struct socket *sock = (struct socket *)socket;
  int err;
  if (level == SOL_SOCKET)
    err = sock_getsockopt(sock, level, optname, optval, optlen);
  else
    err = sock->ops->getsockopt(sock, level, optname, optval, optlen);
  return err;
}
#ifdef NO_ONE_IS_USING_OMG
// Wake Up DCE Waiter
int sim_wake_function(wait_queue_t *wait, unsigned mode, int flags, void *key)
{
  sim_poll_event((int)key, wait->private);

  return 0;
}

// Wait for an event occuring over the socket
void* sim_sock_pollwait (struct SimSocket *socket, void *context)
{
  struct socket *sock = (struct socket *)socket;
  wait_queue_t *wait = ( wait_queue_t * ) sim_malloc (sizeof(wait_queue_t));
  if (!wait)
    return NULL;

  wait->func = sim_wake_function;
  wait->private = context;
  wait->task_list.prev = NULL;
  wait->task_list.next = NULL;

  add_wait_queue ( &sock->sk->sk_wq->wait,  wait);

  return wait;
}

void sim_sock_freepoll (struct SimSocket *socket, void *wait)
{
  struct socket *sock = (struct socket *)socket;
  remove_wait_queue (&sock->sk->sk_wq->wait, wait);
  sim_free (wait);
}
#endif  /* NO_ONE_IS_USING_OMG */

int sim_sock_canrecv (struct SimSocket *socket)
{
  struct socket *sock = (struct socket *)socket;

  switch ( sock->sk->sk_state )
  {
    case TCP_CLOSE:
    if ( SOCK_STREAM == sock->sk->sk_type) return 1;

    case TCP_ESTABLISHED:
      return sock->sk->sk_receive_queue.qlen > 0;

    case TCP_SYN_SENT:
    case TCP_SYN_RECV:
    case TCP_LAST_ACK:
    case TCP_CLOSING:
      return 0;

    case TCP_FIN_WAIT1:
    case TCP_FIN_WAIT2:
    case TCP_TIME_WAIT:
    case TCP_CLOSE_WAIT:
      return 1;

    case TCP_LISTEN:
      {
        struct inet_connection_sock *icsk = inet_csk(sock->sk);

        return !reqsk_queue_empty(&icsk->icsk_accept_queue);
      }

    default: break;
  }

  return 0;
}
int sim_sock_cansend (struct SimSocket *socket)
{
  struct socket *sock = (struct socket *)socket;

  return sock_writeable ( sock->sk );
}

/**
 * Struct used to pass pool table context between DCE and Kernel and back from Kernel to DCE
 *
 * When calling sock_poll we provide in ret field the wanted eventmask, and in the opaque field
 * the DCE poll table
 *
 * if a corresponding event occurs later, the PollEvent will be called by kernel with the DCE
 * poll table in context variable, then we will able to wake up the thread blocked in poll call.
 *
 * Back from sock_poll method the kernel change ret field with the response from poll return of the
 * corresponding kernel socket, and in opaque field there is a reference to the kernel poll table
 * we will use this reference to remove us from the file wait queue when ending the DCE poll call or
 * when ending the DCE process which is curently polling.
 *
 */
struct poll_table_ref {
  int ret;
  void *opaque;
};

// Because the poll main loop code is in NS3/DCE we have only on entry in our kernel poll table
struct sim_ptable_entry
{
  wait_queue_t wait;
  wait_queue_head_t *wait_address;
  int eventMask;  // Poll wanted event mask.
  void *opaque; // Pointeur to DCE poll table
};

static int sim_pollwake(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
  struct sim_ptable_entry *entry = (struct sim_ptable_entry *) wait->private;

  if ( ((int)key) & entry->eventMask)
    { // Filter only wanted events
      sim_poll_event((int)key, entry->opaque);
    }

  return 0;
}

static void sim_pollwait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p)
{
  struct poll_wqueues *pwq = container_of(p, struct poll_wqueues, pt);
  struct sim_ptable_entry *entry =
      ( struct sim_ptable_entry * ) sim_malloc (sizeof(struct sim_ptable_entry));
  struct poll_table_ref *fromDCE =  (struct poll_table_ref *) pwq->table;

  if (!entry)
    return;

  entry->opaque = fromDCE->opaque; // Copy DCE poll table reference
  entry->eventMask = fromDCE->ret; // Copy poll mask of wanted events.

  pwq->table = entry;

  init_waitqueue_func_entry(&entry->wait, sim_pollwake);
  entry->wait.private = entry;
  entry->wait_address = wait_address;
  add_wait_queue(wait_address, &entry->wait);
}

void dce_poll_initwait(struct poll_wqueues *pwq)
{
  init_poll_funcptr(&pwq->pt, sim_pollwait);
  pwq->polling_task = current;
  pwq->triggered = 0;
  pwq->error = 0;
  pwq->table = NULL;
  pwq->inline_index = 0;
}

// call poll on socket ...
void sim_sock_poll (struct SimSocket *socket, struct poll_table_ref *ret)
{
  struct socket *sock = (struct socket *)socket;
  // Provide a fake file structure
  struct file zero;
  poll_table *pwait = 0;
  struct poll_wqueues *ptable = 0;

  sim_memset(&zero,0,sizeof(struct file));

  if ( ret->opaque )
    {
      ptable = (struct poll_wqueues *)sim_malloc (sizeof(struct poll_wqueues));
      if (!ptable)
        return;

      dce_poll_initwait(ptable);

      pwait = &(ptable->pt);
      // Pass the DCE pool table to sim_pollwait function
      ptable->table = ret;
    }

  ret->ret = sock->ops->poll(&zero, sock, pwait);
  // Pass back the kernel poll table to DCE in order to DCE to remove from wait queue
  // using sim_sock_pollfreewait method below
  ret->opaque = ptable;
}

void sim_sock_pollfreewait (void *polltable)
{
  struct poll_wqueues* ptable = (struct poll_wqueues*) polltable;

  if (ptable && ptable->table)
    {
      struct sim_ptable_entry *entry = ( struct sim_ptable_entry * )ptable->table;
      remove_wait_queue(entry->wait_address, &entry->wait);
      sim_free (entry);
    }
  sim_free (ptable);
}




