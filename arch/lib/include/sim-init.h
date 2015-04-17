/*
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef SIM_INIT_H
#define SIM_INIT_H

#include <linux/socket.h>
#include "sim-types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _IO_FILE;
typedef struct _IO_FILE FILE;

struct SimExported {
	struct SimTask *(*task_create)(void *priv, unsigned long pid);
	void (*task_destroy)(struct SimTask *task);
	void *(*task_get_private)(struct SimTask *task);

	int (*sock_socket)(int domain, int type, int protocol,
			struct SimSocket **socket);
	int (*sock_close)(struct SimSocket *socket);
	ssize_t (*sock_recvmsg)(struct SimSocket *socket, struct msghdr *msg,
				int flags);
	ssize_t (*sock_sendmsg)(struct SimSocket *socket,
				const struct msghdr *msg, int flags);
	int (*sock_getsockname)(struct SimSocket *socket,
				struct sockaddr *name, int *namelen);
	int (*sock_getpeername)(struct SimSocket *socket,
				struct sockaddr *name, int *namelen);
	int (*sock_bind)(struct SimSocket *socket, const struct sockaddr *name,
			int namelen);
	int (*sock_connect)(struct SimSocket *socket,
			const struct sockaddr *name, int namelen,
			int flags);
	int (*sock_listen)(struct SimSocket *socket, int backlog);
	int (*sock_shutdown)(struct SimSocket *socket, int how);
	int (*sock_accept)(struct SimSocket *socket,
			struct SimSocket **newSocket, int flags);
	int (*sock_ioctl)(struct SimSocket *socket, int request, char *argp);
	int (*sock_setsockopt)(struct SimSocket *socket, int level,
			int optname,
			const void *optval, int optlen);
	int (*sock_getsockopt)(struct SimSocket *socket, int level,
			int optname,
			void *optval, int *optlen);

	void (*sock_poll)(struct SimSocket *socket, void *ret);
	void (*sock_pollfreewait)(void *polltable);

	struct SimDevice *(*dev_create)(const char *ifname, void *priv,
					enum SimDevFlags flags);
	void (*dev_destroy)(struct SimDevice *dev);
	void *(*dev_get_private)(struct SimDevice *task);
	void (*dev_set_address)(struct SimDevice *dev,
				unsigned char buffer[6]);
	void (*dev_set_mtu)(struct SimDevice *dev, int mtu);
	struct SimDevicePacket (*dev_create_packet)(struct SimDevice *dev,
						int size);
	void (*dev_rx)(struct SimDevice *dev, struct SimDevicePacket packet);

	void (*sys_iterate_files)(const struct SimSysIterator *iter);
	int (*sys_file_read)(const struct SimSysFile *file, char *buffer,
			int size, int offset);
	int (*sys_file_write)(const struct SimSysFile *file,
			const char *buffer, int size, int offset);
};

struct SimImported {
	int (*vprintf)(struct SimKernel *kernel, const char *str,
		va_list args);
	void *(*malloc)(struct SimKernel *kernel, unsigned long size);
	void (*free)(struct SimKernel *kernel, void *buffer);
	void *(*memcpy)(struct SimKernel *kernel, void *dst, const void *src,
			unsigned long size);
	void *(*memset)(struct SimKernel *kernel, void *dst, char value,
			unsigned long size);
	int (*atexit)(struct SimKernel *kernel, void (*function)(void));
	int (*access)(struct SimKernel *kernel, const char *pathname,
		int mode);
	char *(*getenv)(struct SimKernel *kernel, const char *name);
	int (*mkdir)(struct SimKernel *kernel, const char *pathname,
		mode_t mode);
	int (*open)(struct SimKernel *kernel, const char *pathname, int flags);
	int (*__fxstat)(struct SimKernel *kernel, int ver, int fd, void *buf);
	int (*fseek)(struct SimKernel *kernel, FILE *stream, long offset,
		int whence);
	void (*setbuf)(struct SimKernel *kernel, FILE *stream, char *buf);
	FILE *(*fdopen)(struct SimKernel *kernel, int fd, const char *mode);
	long (*ftell)(struct SimKernel *kernel, FILE *stream);
	int (*fclose)(struct SimKernel *kernel, FILE *fp);
	size_t (*fread)(struct SimKernel *kernel, void *ptr, size_t size,
			size_t nmemb, FILE *stream);
	size_t (*fwrite)(struct SimKernel *kernel, const void *ptr, size_t size,
			 size_t nmemb, FILE *stream);

	unsigned long (*random)(struct SimKernel *kernel);
	void *(*event_schedule_ns)(struct SimKernel *kernel, __u64 ns,
				void (*fn)(void *context), void *context,
				void (*pre_fn)(void));
	void (*event_cancel)(struct SimKernel *kernel, void *event);
	__u64 (*current_ns)(struct SimKernel *kernel);

	struct SimTask *(*task_start)(struct SimKernel *kernel,
				void (*callback)(void *),
				void *context);
	void (*task_wait)(struct SimKernel *kernel);
	struct SimTask *(*task_current)(struct SimKernel *kernel);
	int (*task_wakeup)(struct SimKernel *kernel, struct SimTask *task);
	void (*task_yield)(struct SimKernel *kernel);

	void (*dev_xmit)(struct SimKernel *kernel, struct SimDevice *dev,
			unsigned char *data, int len);
	void (*signal_raised)(struct SimKernel *kernel, struct SimTask *task,
			int sig);
	void (*poll_event)(int flag, void *context);
};

typedef void (*SimInit)(struct SimExported *, const struct SimImported *,
			struct SimKernel *kernel);
void sim_init(struct SimExported *exported, const struct SimImported *imported,
	struct SimKernel *kernel);

#ifdef __cplusplus
}
#endif

#endif /* SIM_INIT_H */
