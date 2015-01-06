#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "nuse-hostcalls.h"
#include "nuse-vif.h"
#include "nuse.h"

#define NUSE_DEFAULT_PIPE_PRIV	0666

static int
named_pipe_alloc(const char *path)
{
	int fd;

	if (mkfifo(path, NUSE_DEFAULT_PIPE_PRIV) < 0) {
		perror ("mkfifo");
		return -1;
	}

	if ((fd = host_open(path, O_RDWR)) < 0) {
		perror ("open");
		return -1;
	}

	return fd;
}

void
nuse_vif_pipe_read(struct nuse_vif *vif, struct SimDevice *dev)
{
	int sock = vif->sock;
	char buf[8192];
	ssize_t size;

	while (1) {
		size = host_read(sock, buf, sizeof(buf));
		if (size < 0) {
			perror("read");
			host_close(sock);
			return;
		} else if (size == 0) {
			host_close(sock);
			return;
		}

		nuse_dev_rx(dev, buf, size);
	}
}

void
nuse_vif_pipe_write(struct nuse_vif *vif, struct SimDevice *dev,
		    unsigned char *data, int len)
{
	int sock = vif->sock;
	int ret = host_write(sock, data, len);

	if (ret == -1)
		perror ("write");
}

void *
nuse_vif_pipe_create(const char *pipepath)
{
	int sock, n;
	struct nuse_vif *vif;

	sock = named_pipe_alloc(pipepath);
	if (sock < 0) {
		printf ("failed to create named pipe \"%s\"\n", pipepath);
		return NULL;
	}

	vif = malloc (sizeof(struct nuse_vif));
	vif->sock = sock;
	vif->type = NUSE_VIF_PIPE;

	return vif;
}

void
nuse_vif_pipe_delete(struct nuse_vif *vif)
{
	int sock = vif->sock;
	free(vif);
	host_close(sock);
}

static struct nuse_vif_impl nuse_vif_pipe = {
	.read = nuse_vif_pipe_read,
	.write = nuse_vif_pipe_write,
	.create = nuse_vif_pipe_create,
	.delete = nuse_vif_pipe_delete,
};

extern struct nuse_vif_impl *nuse_vif[NUSE_VIF_MAX];

int
nuse_vif_pipe_init(void)
{
	nuse_vif[NUSE_VIF_PIPE] = &nuse_vif_pipe;
	return 0;
}
__define_initcall(nuse_vif_pipe_init, 1);
