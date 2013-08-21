#include "sim.h"
#include "sim-assert.h"
#include <linux/fs.h>

ssize_t generic_file_splice_read(struct file *in, loff_t *ppos,
				 struct pipe_inode_info *pipe, size_t len,
				 unsigned int flags)
{
	sim_assert (false);

	return 0;
}
