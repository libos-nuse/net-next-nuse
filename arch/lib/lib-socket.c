/*
 * socket feature for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#include "sim-init.h"
#include "sim.h"
#include <linux/net.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <net/sock.h>
#include <net/tcp_states.h>
#include <net/inet_connection_sock.h>

#define READ 0
#define WRITE 1

struct SimSocket {};
struct  socket;


static struct iovec *copy_iovec(const struct iovec *input, int len)
{
	int size = sizeof(struct iovec) * len;
	struct iovec *output = lib_malloc(size);

	if (!output)
		return NULL;
	lib_memcpy(output, input, size);
	return output;
}

int lib_sock_socket (int domain, int type, int protocol, struct SimSocket **socket)
{
  printk("Domain : %d",domain);
  struct socket **kernel_socket = (struct socket **)socket;
  int flags;  

  flags = type & ~SOCK_TYPE_MASK;
	if (flags & ~(SOCK_CLOEXEC | SOCK_NONBLOCK))
		return -EINVAL;
	type &= SOCK_TYPE_MASK;
  
  int retval = sock_create(domain, type, protocol, kernel_socket);
  struct file *fp = lib_malloc(sizeof(struct file));
  (*kernel_socket)->file = fp;
  fp->f_cred = lib_malloc(sizeof(struct cred));
  return retval;
}

int lib_sock_close (struct SimSocket *socket)
{
  struct socket *kernel_socket = (struct socket *)socket;
  sock_release(kernel_socket);
  return 0;
}

static size_t iov_size(const struct user_msghdr *msg)
{
	size_t i;
	size_t size = 0;

	for (i = 0; i < msg->msg_iovlen; i++)
		size += msg->msg_iov[i].iov_len;
	return size;
}

ssize_t lib_sock_recvmsg (struct SimSocket *socket, struct user_msghdr *msg, int flags)
{
	struct socket *kernel_socket = (struct socket *)socket;
	struct msghdr msg_sys;
	struct cmsghdr *user_cmsgh = msg->msg_control;
	size_t user_cmsghlen = msg->msg_controllen;
	int retval;

	msg_sys.msg_name = msg->msg_name;
	msg_sys.msg_namelen = msg->msg_namelen;
	msg_sys.msg_control = msg->msg_control;
	msg_sys.msg_controllen = msg->msg_controllen;
	msg_sys.msg_flags = flags;

	iov_iter_init(&msg_sys.msg_iter, READ,
		msg->msg_iov, msg->msg_iovlen, iov_size(msg));

	retval = sock_recvmsg(kernel_socket, &msg_sys , flags);

	msg->msg_name = msg_sys.msg_name;
	msg->msg_namelen = msg_sys.msg_namelen;
	msg->msg_control = user_cmsgh;
	msg->msg_controllen = user_cmsghlen - msg_sys.msg_controllen;
	return retval;
}


ssize_t lib_sock_sendmsg (struct SimSocket *socket, const struct user_msghdr *msg, int flags)
{
struct socket *kernel_socket = (struct socket *)socket;
	struct iovec *kernel_iov = copy_iovec(msg->msg_iov, msg->msg_iovlen);
	struct msghdr msg_sys;
	int retval;

	msg_sys.msg_name = msg->msg_name;
	msg_sys.msg_namelen = msg->msg_namelen;
	msg_sys.msg_control = msg->msg_control;
	msg_sys.msg_controllen = msg->msg_controllen;
	msg_sys.msg_flags = flags;

	iov_iter_init(&msg_sys.msg_iter, WRITE,
		kernel_iov, msg->msg_iovlen, iov_size(msg));

	retval = sock_sendmsg(kernel_socket, &msg_sys);    
	lib_free(kernel_iov);
	return retval;
}

int lib_sock_getsockname (struct SimSocket *socket, struct sockaddr *name, int *namelen)
{
  struct socket *kernel_socket = (struct socket *)socket;
  int error = kernel_socket->ops->getname(kernel_socket, name, 0);
  return error;
}

int lib_sock_getpeername (struct SimSocket *socket, struct sockaddr *name, int *namelen)
{
  struct socket *kernel_socket = (struct socket *)socket;
  int error = kernel_socket->ops->getname(kernel_socket, name, 1);

  return error;
}

int lib_sock_bind (struct SimSocket *socket, const struct sockaddr *name, int namelen)
{
  struct socket * kernel_socket = (struct socket *)socket;
  struct sockaddr_storage address;

  memcpy(&address, name, namelen);
  int error = kernel_socket->ops->bind(kernel_socket, (struct sockaddr *)&address, namelen);
  return error;
}

int lib_sock_connect (struct SimSocket *socket, const struct sockaddr *name, int namelen, int flags)
{
  struct socket *kernel_socket = (struct socket *)socket;
  printk("Sock addr : %s",name->sa_data);
  printk("Sock family : %s",name->sa_family);
  printk("Working on socket with domain : %d",kernel_socket->sk->sk_protocol);
  struct sockaddr_storage address;

  //move_addr_to_kernel(name, namelen, &address);
  memcpy(&address, name, namelen);

  //kernel_socket->file->f_flags = kernel_socket->file->f_flags | flags;
  kernel_socket->file->f_flags = flags;
  int retval = kernel_socket->ops->connect(kernel_socket, (struct sockaddr *)&address,
          namelen,  flags);
  return retval;
}

int lib_sock_listen (struct SimSocket *socket, int backlog)
{
  struct socket * kernel_socket = (struct socket *)socket;
  int error = kernel_socket->ops->listen(kernel_socket, backlog);
  return error;
}

int lib_sock_shutdown (struct SimSocket *socket, int how)
{
  struct socket *kernel_socket = (struct socket *)socket;
  int retval = kernel_socket->ops->shutdown(kernel_socket, how);
  return retval;
}

int lib_sock_accept (struct SimSocket *socket, struct SimSocket **new_socket, int flags)
{
  struct socket *sock, *newsock;
  int err;

  sock = (struct socket *)socket;

  /* the fields do not matter here. If we could, */
  /* we would call sock_alloc but it's not exported. */
  err = sock_create_lite(0, 0, 0, &newsock);
  if (err < 0)
    return err;
  newsock->type = sock->type;
  newsock->ops = sock->ops;

  err = sock->ops->accept(sock, newsock, flags, false);
  if (err < 0) {
    sock_release(newsock);
    return err;
  }
  *new_socket = (struct SimSocket *)newsock;
  return 0;
}

int lib_sock_ioctl (struct SimSocket *socket, int request, char *argp)
{
  struct socket *sock = (struct socket *)socket;
	struct sock *sk;
	struct net *net;
  void __user *arg = (void __user *)argp;
  struct ifreq ifr;
	bool need_copyout;
	int err;

	sk = sock->sk;
	net = sock_net(sk);

	err = sock->ops->ioctl(sock, request, (long)argp);

	/*
	 * If this ioctl is unknown try to hand it down
	 * to the NIC driver.
	 */
  if (copy_from_user(&ifr, arg, sizeof(struct ifreq)))
			return -EFAULT;

	if (err == -ENOIOCTLCMD)
		err = dev_ioctl(net,request,&ifr,&need_copyout);
  
	return err;  
}

int lib_sock_setsockopt (struct SimSocket *socket, int level, int optname, const void *optval, int optlen)
{
  struct socket *kernel_socket = (struct socket *)socket;
  char *coptval = (char *)optval;
  int error;

  if (level == SOL_SOCKET)
    error = sock_setsockopt(kernel_socket, level, optname, USER_SOCKPTR(coptval), optlen);
  else
	error = kernel_socket->ops->setsockopt(kernel_socket, level, optname, USER_SOCKPTR(coptval), optlen);

  return error;
}

int lib_sock_getsockopt (struct SimSocket *socket, int level, int optname, void *optval, int *optlen)
{
  struct socket *kernel_socket = (struct socket *)socket;
  int error;
	
  if(level == SOL_SOCKET)
  	error = sock_getsockopt(kernel_socket, level, optname, optval, optlen);
  else
	error = kernel_socket->ops->getsockopt(kernel_socket, level, optname, optval, optlen);

  return error;
}

struct poll_table_ref {
	int ret;
	void *opaque;
};

/* Because the poll main loop code is in NS3/DCE we have only on entry
   in our kernel poll table */
struct lib_ptable_entry {
	wait_queue_entry_t wait;
	wait_queue_head_t *wait_address;
	int eventMask;  /* Poll wanted event mask. */
	void *opaque;   /* Pointeur to DCE poll table */
};

static int lib_pollwake(wait_queue_entry_t *wait, unsigned mode, int sync, void *key)
{
	struct lib_ptable_entry *entry =
		(struct lib_ptable_entry *)wait->private;

	/* Filter only wanted events */
	if (key && !((unsigned long)key & entry->eventMask))
		return 0;

	lib_poll_event((unsigned long)key, entry->opaque);
	return 1;
}

static void lib_pollwait(struct file *filp, wait_queue_head_t *wait_address,
			 poll_table *p)
{
	struct poll_wqueues *pwq = container_of(p, struct poll_wqueues, pt);
	struct lib_ptable_entry *entry =
		(struct lib_ptable_entry *)
		lib_malloc(sizeof(struct lib_ptable_entry));
	struct poll_table_ref *fromDCE =  (struct poll_table_ref *)pwq->table;

	if (!entry)
		return;

	entry->opaque = fromDCE->opaque; /* Copy DCE poll table reference */
	entry->eventMask = fromDCE->ret; /* Copy poll mask of wanted events. */

	pwq->table = (struct poll_table_page *)entry;

	init_waitqueue_func_entry(&entry->wait, lib_pollwake);
	entry->wait.private = entry;
	entry->wait_address = wait_address;
	add_wait_queue(wait_address, &entry->wait);
}

void dce_poll_initwait(struct poll_wqueues *pwq)
{
	init_poll_funcptr(&pwq->pt, lib_pollwait);
	pwq->polling_task = current;
	pwq->triggered = 0;
	pwq->error = 0;
	pwq->table = NULL;
	pwq->inline_index = 0;
}

/* call poll on socket ... */
void lib_sock_poll(struct SimSocket *socket, struct poll_table_ref *ret)
{
	struct socket *sock = (struct socket *)socket;
	/* Provide a fake file structure */
	struct file zero;
	poll_table *pwait = 0;
	struct poll_wqueues *ptable = 0;

	lib_memset(&zero, 0, sizeof(struct file));

	if (ret->opaque) {
		ptable =
			(struct poll_wqueues *)lib_malloc(sizeof(struct
								 poll_wqueues));
		if (!ptable)
			return;

		dce_poll_initwait(ptable);

		pwait = &(ptable->pt);
		/* Pass the DCE pool table to lib_pollwait function */
		ptable->table = (struct poll_table_page *)ret;
	}

	ret->ret = sock->ops->poll(&zero, sock, pwait);
	/* Pass back the kernel poll table to DCE in order to DCE to */
	/* remove from wait queue */
	/* using lib_sock_pollfreewait method below */
	ret->opaque = ptable;
}

void lib_sock_pollfreewait(void *polltable)
{
	struct poll_wqueues *ptable = (struct poll_wqueues *)polltable;

	if (ptable && ptable->table) {
		struct lib_ptable_entry *entry =
			(struct lib_ptable_entry *)ptable->table;
		remove_wait_queue(entry->wait_address, &entry->wait);
		lib_free(entry);
	}
	lib_free(ptable);
}


