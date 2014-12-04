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
	lib_init (exported, imported, kernel);
	exported->dev_create = sim_dev_create_forwarder;
}
