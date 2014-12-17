#include <linux/init.h>         /* initcall_t */
#include <linux/kernel.h>       /* SYSTEM_BOOTING */
#include <linux/sched.h>        /* struct task_struct */
#include <linux/device.h>
#include <drivers/base/base.h>
#include <include/linux/idr.h>
#include <include/linux/rcupdate.h>
#include <stdio.h>
#include "sim-init.h"
#include "sim.h"

FILE *stderr = NULL;

extern struct SimImported g_imported;
extern struct SimKernel *g_kernel;

static int num_handler = 0;
void *atexit_list[1024];

extern struct SimDevice *lib_dev_create(char *ifname, void *priv,
					enum SimDevFlags flags);

extern void lib_init(struct SimExported *exported,
		const struct SimImported *imported,
		struct SimKernel *kernel);

struct SimDevice *sim_dev_create_forwarder(void *priv, enum SimDevFlags flags)
{
	return lib_dev_create("sim%d", priv, flags);
}

void sim_init(struct SimExported *exported, const struct SimImported *imported,
	      struct SimKernel *kernel)
{
	int i;

	lib_init (exported, imported, kernel);
	exported->dev_create = sim_dev_create_forwarder;

	/* XXX handle atexit registration for gcov */
	for (i = 0; i < 1024; i++) {
		if (atexit_list[i]) {
			g_imported.atexit(g_kernel,
					(void (*)(void))atexit_list[i]);
		}
	}

}

int fclose(FILE *fp)
{
	return g_imported.fclose(g_kernel, fp);
}
char *getenv(const char *name)
{
	return g_imported.getenv(g_kernel, name);
}
int access(const char *pathname, int mode)
{
	return g_imported.access(g_kernel, pathname, mode);
}
int atexit(void (*function)(void))
{
	if (g_imported.atexit == 0) {
		atexit_list[num_handler++] = function;
		return 0;
	} else {
		return g_imported.atexit(g_kernel, function);
	}
}
pid_t getpid(void)
{
	return (pid_t)0;
}
int mkdir(const char *pathname, mode_t mode)
{
	return g_imported.mkdir(g_kernel, pathname, mode);
}
int open(const char *pathname, int flags)
{
	return g_imported.open(g_kernel, pathname, flags);
}
int fcntl(int fd, int cmd, ... /* arg */)
{
	return 0;
}
int __fxstat(int ver, int fd, void *buf)
{
	return g_imported.__fxstat(g_kernel, ver, fd, buf);
}
int fseek(FILE *stream, long offset, int whence)
{
	return g_imported.fseek(g_kernel, stream, offset, whence);
}
long ftell(FILE *stream)
{
	return g_imported.ftell(g_kernel, stream);
}
void setbuf(FILE *stream, char *buf)
{
	return g_imported.setbuf(g_kernel, stream, buf);
}
FILE *fdopen(int fd, const char *mode)
{
	return g_imported.fdopen(g_kernel, fd, mode);
}
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return g_imported.fread(g_kernel, ptr, size, nmemb, stream);
}
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return g_imported.fwrite(g_kernel, ptr, size, nmemb, stream);
}
