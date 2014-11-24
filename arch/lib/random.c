#include "sim.h"
#include <linux/random.h>

u32 random32(void)
{
	return lib_random();
}

void get_random_bytes(void *buf, int nbytes)
{
	char *p = (char *)buf;
	int i;

	for (i = 0; i < nbytes; i++)
		p[i] = lib_random();
}
void srandom32(u32 entropy)
{
}

u32 prandom_u32(void)
{
	return lib_random();
}
void prandom_seed(u32 entropy)
{
}

void prandom_bytes(void *buf, size_t bytes)
{
	return get_random_bytes(buf, bytes);
}

#include <linux/sysctl.h>

static int nothing;
struct ctl_table random_table[] = {
	{
		.procname       = "nothing",
		.data           = &nothing,
		.maxlen         = sizeof(int),
		.mode           = 0444,
		.proc_handler   = proc_dointvec,
	}
};
