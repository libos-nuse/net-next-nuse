/*
 * host system/library calls for NUSE
 * Copyright (c) 2015 Hajime Tazaki
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#define _GNU_SOURCE /* Get RTLD_NEXT */
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/stat.h>
#include <assert.h>
#include "nuse-hostcalls.h"

/* nuse-hostcalls.c */
int (*host_pthread_create)(pthread_t *, const struct pthread_attr *,
			   void *(*)(void *), void *) = NULL;
int (*host_pthread_join)(pthread_t thread, void **retval) = NULL;
int (*host_poll)(struct pollfd *, int, int) = NULL;
int (*host_socket)(int fd, int type, int proto) = NULL;
int (*host_close)(int fd) = NULL;
int (*host_bind)(int, const struct sockaddr *, int) = NULL;
ssize_t (*host_write)(int fd, const void *buf, size_t count) = NULL;
ssize_t (*host_send)(int sockfd, const void *buf, size_t len, int flags) = NULL;
ssize_t (*host_sendmsg)(int sockfd, const struct msghdr *msg, int flags) = NULL;
ssize_t (*host_read)(int fd, void *buf, size_t count) = NULL;
ssize_t (*host_writev)(int fd, const struct iovec *iovec, size_t count) = NULL;
int (*host_open)(const char *pathname, int flags,...) = NULL;
int (*host_open64)(const char *pathname, int flags,...) = NULL;
int (*host_ioctl)(int d, int request, ...) = NULL;
char *(*host_getenv)(const char *name) = NULL;
int (*host_fclose)(FILE *fp) = NULL;
FILE *(*host_fdopen)(int fd, const char *mode) = NULL;
int (*host_fcntl)(int fd, int cmd, ... /* arg */ ) = NULL;
size_t (*host_fwrite)(const void *ptr, size_t size, size_t nmemb,
		FILE *stream) = NULL;
int (*host_access)(const char *pathname, int mode) = NULL;
int (*host_pipe)(int pipefd[2]) = NULL;
int (*host_listen)(int sockfd, int backlog) = NULL;
int (*host_accept)(int sockfd, struct sockaddr *addr, int *addrlen) = NULL;
int (*host_getsockopt)(int sockfd, int level, int optname,
		void *optval, int *optlen) = NULL;
int (*host_setsockopt)(int sockfd, int level, int optname,
		const void *optval, int optlen) = NULL;
pid_t (*host_getpid)(void) = NULL;

static void *
nuse_hostcall_resolve_sym(const char *sym)
{
	void *resolv;

	resolv = dlsym(RTLD_NEXT, sym);
	if (!resolv) {
		printf("dlsym fail %s (%s)\n", sym, dlerror());
		assert(0);
	}
	return resolv;
}

void nuse_hostcall_init(void)
{
	/* host functions */
	host_socket = nuse_hostcall_resolve_sym("socket");
	host_send = nuse_hostcall_resolve_sym("send");
	host_sendmsg = nuse_hostcall_resolve_sym("sendmsg");
	host_write = nuse_hostcall_resolve_sym("write");
	host_writev = nuse_hostcall_resolve_sym("writev");
	host_read = nuse_hostcall_resolve_sym("read");
	host_listen = nuse_hostcall_resolve_sym("listen");
	host_accept = nuse_hostcall_resolve_sym("accept");
	host_close = nuse_hostcall_resolve_sym("close");
	host_bind = nuse_hostcall_resolve_sym("bind");
	host_pthread_create = nuse_hostcall_resolve_sym("pthread_create");
	host_pthread_join = nuse_hostcall_resolve_sym("pthread_join");
	host_poll = nuse_hostcall_resolve_sym("poll");
	host_open = nuse_hostcall_resolve_sym("open");
	host_open64 = nuse_hostcall_resolve_sym("open64");
	host_ioctl = nuse_hostcall_resolve_sym("ioctl");
	host_pipe = nuse_hostcall_resolve_sym("pipe");
	host_getenv = nuse_hostcall_resolve_sym("getenv");
	host_fdopen = nuse_hostcall_resolve_sym("fdopen");
	host_fcntl = nuse_hostcall_resolve_sym("fcntl");
	host_fclose = nuse_hostcall_resolve_sym("fclose");
	host_fwrite = nuse_hostcall_resolve_sym("fwrite");
	host_access = nuse_hostcall_resolve_sym("access");
	host_getpid = nuse_hostcall_resolve_sym("getpid");
	host_setsockopt = nuse_hostcall_resolve_sym("setsockopt");
	host_getsockopt = nuse_hostcall_resolve_sym("getsockopt");

}

