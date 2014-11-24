#include <linux/pid.h>
#include "sim.h"
#include "sim-assert.h"

void put_pid(struct pid *pid)
{
}
pid_t pid_vnr(struct pid *pid)
{
	return pid_nr(pid);
}
struct task_struct *find_task_by_vpid(pid_t nr)
{
	lib_assert(false);
	return 0;
}
struct pid *find_get_pid(int nr)
{
	lib_assert(false);
	return 0;
}
