#ifndef LIB_H
#define LIB_H

#include <linux/sched.h>

struct SimTask {
	struct list_head head;
	struct task_struct kernel_task;
	void *private;
};

#endif /* LIB_H */
