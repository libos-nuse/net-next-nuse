#include "sim.h"
#include "sim-assert.h"
#include <linux/fs.h>


ssize_t generic_file_aio_read(struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d)
{
	sim_assert (false);

	return 0;
}

int generic_file_readonly_mmap(struct file * file, struct vm_area_struct * vma)
{
	return -ENOSYS;
}

