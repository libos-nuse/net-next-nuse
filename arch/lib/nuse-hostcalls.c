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
int (*host_pthread_create)(struct pthread *, const struct pthread_attr *,
			   void *(*)(void *), void *) = NULL;
int (*host_poll)(struct pollfd *, int, int) = NULL;
int (*host_socket)(int fd, int type, int proto) = NULL;
int (*host_close)(int fd) = NULL;
int (*host_bind)(int, const struct sockaddr *, int) = NULL;
ssize_t (*host_write)(int fd, const void *buf, size_t count) = NULL;
ssize_t (*host_read)(int fd, void *buf, size_t count) = NULL;
ssize_t (*host_writev)(int fd, const struct iovec *iovec, size_t count) = NULL;
int (*host_open)(const char *pathname, int flags,...) = NULL;
int (*host_open64)(const char *pathname, int flags,...) = NULL;
int (*host_ioctl)(int d, int request, ...) = NULL;
char *(*host_getenv)(const char *name) = NULL;
int (*host_fclose)(FILE *fp) = NULL;
FILE *(*host_fdopen)(int fd, const char *mode) = NULL;

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
	host_write = nuse_hostcall_resolve_sym("write");
	host_writev = nuse_hostcall_resolve_sym("writev");
	host_read = nuse_hostcall_resolve_sym("read");
	host_close = nuse_hostcall_resolve_sym("close");
	host_bind = nuse_hostcall_resolve_sym("bind");
	host_pthread_create = nuse_hostcall_resolve_sym("pthread_create");
	host_poll = nuse_hostcall_resolve_sym("poll");
	host_open = nuse_hostcall_resolve_sym("open");
	host_open64 = nuse_hostcall_resolve_sym("open64");
	host_ioctl = nuse_hostcall_resolve_sym("ioctl");
	host_getenv = nuse_hostcall_resolve_sym("getenv");
	host_fdopen = nuse_hostcall_resolve_sym("fdopen");
	host_fclose = nuse_hostcall_resolve_sym("fclose");

}

