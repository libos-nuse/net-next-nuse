#include <linux/fs.h>
#include <linux/splice.h>
#include <linux/mount.h>
#include <linux/sysctl.h>
#include <fs/mount.h>
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/pagemap.h>
#include <linux/user_namespace.h>
#include <linux/lglock.h>

#include "sim-assert.h"

__cacheline_aligned_in_smp DEFINE_SEQLOCK(mount_lock);
struct super_block;

struct backing_dev_info default_backing_dev_info = {
	.name		= "default",
	.ra_pages	= VM_MAX_READAHEAD * 1024 / PAGE_CACHE_SIZE,
	.state		= 0,
	.capabilities	= BDI_CAP_MAP_COPY,
};
struct user_namespace init_user_ns ;

int get_sb_pseudo(struct file_system_type *type, char *str,
		  const struct super_operations *ops, unsigned long z,
		  struct vfsmount *mnt)
{
  // called from sockfs_get_sb by kern_mount_data
  mnt->mnt_sb->s_root = 0;
  mnt->mnt_sb->s_op = ops;
  return 0;
}
struct inode *new_inode(struct super_block *sb)
{
  // call sock_alloc_inode through s_op
  struct inode *inode = sb->s_op->alloc_inode(sb);
  inode->i_ino = 0;
  inode->i_sb = sb;
  atomic_set (&inode->i_count, 1);
  inode->i_state = 0;
  return inode;
}
void iput(struct inode *inode)
{
  if (atomic_dec_and_test (&inode->i_count))
    {
      // call sock_destroy_inode
      inode->i_sb->s_op->destroy_inode (inode);
    }
}
void inode_init_once(struct inode *inode)
{
  memset(inode, 0, sizeof(*inode));
}

// Implementation taken from vfs_kern_mount from linux/namespace.c
struct vfsmount *kern_mount_data(struct file_system_type *type, void *data)
{
	static struct mount local_mnt;
	struct mount *mnt = &local_mnt;
	struct dentry *root = 0;

	memset (mnt,0,sizeof (struct mount));
	if (!type)
		return ERR_PTR(-ENODEV);
	int flags = MS_KERNMOUNT;
	char *name = (char *)type->name;
	if (flags & MS_KERNMOUNT)
		mnt->mnt.mnt_flags = MNT_INTERNAL;

	root = type->mount(type, flags, name, data);
	if (IS_ERR(root)) {
		return ERR_CAST(root);
	}

	mnt->mnt.mnt_root = root;
	mnt->mnt.mnt_sb = root->d_sb;
	mnt->mnt_mountpoint = mnt->mnt.mnt_root;
	mnt->mnt_parent = mnt;
//	br_write_lock(vfsmount_lock);   DCE is monothreaded , so we do not care of lock here
	list_add_tail(&mnt->mnt_instance, &root->d_sb->s_mounts);
//	br_write_unlock(vfsmount_lock);  DCE is monothreaded , so we do not care of lock here

	return &mnt->mnt;
}

int register_filesystem(struct file_system_type *fs)
{
  // We don't need to register anything because we never really implement 
  // any kind of filesystem. return 0 to signal success.
  return 0;
}

int alloc_fd(unsigned start, unsigned flags)
{
  sim_assert (false);
  return 0;
}
void fd_install(unsigned int fd, struct file *file)
{
  sim_assert (false);
}
void put_unused_fd(unsigned int fd)
{
  sim_assert (false);
}

struct file *alloc_file(struct path *path, fmode_t mode,
			const struct file_operations *fop)
{
  sim_assert (false);
  return 0;
}

struct file *fget(unsigned int fd)
{
  sim_assert (false);
  return 0;
}
struct file *fget_light(unsigned int fd, int *fput_needed)
{
  sim_assert (false);
  return 0;
}
void fput(struct file *file)
{
  sim_assert (false);
}

struct dentry * d_alloc(struct dentry *entry, const struct qstr *str)
{
  sim_assert (false);
  return 0;
}
void d_instantiate(struct dentry *entry, struct inode *inode)
{
}
char *dynamic_dname(struct dentry *dentry, char *buffer, int buflen,
		    const char *fmt, ...)
{
  sim_assert (false);
  return 0;
}

struct dentry_stat_t dentry_stat;
struct files_stat_struct files_stat;
struct inodes_stat_t inodes_stat;

pid_t f_getown(struct file *filp)
{
  sim_assert (false);
  return 0;
}
int f_setown(struct file *filp, unsigned long arg, int force)
{
  sim_assert (false);
  return 0;
}


void kill_fasync(struct fasync_struct **fs, int a, int b)
{
  sim_assert (false);
}
int fasync_helper(int a, struct file *file, int b, struct fasync_struct **c)
{
  sim_assert (false);
  return 0;
}
long sys_close(unsigned int fd)
{
  sim_assert (false);
  return 0;
}
ssize_t splice_to_pipe(struct pipe_inode_info *info,
		       struct splice_pipe_desc *desc)
{
  sim_assert (false);
  return 0;
}
int splice_grow_spd(const struct pipe_inode_info *info, struct splice_pipe_desc *desc)
{
  sim_assert (false);
  return 0;
}
void splice_shrink_spd(struct splice_pipe_desc *desc)
{
  sim_assert (false);
}

ssize_t generic_splice_sendpage(struct pipe_inode_info *pipe,
				struct file *out, loff_t *poff, size_t len, unsigned int flags)
{
  sim_assert (false);
  return 0;
}
void *generic_pipe_buf_map(struct pipe_inode_info *pipe,
                           struct pipe_buffer *buf, int atomic)
{
  sim_assert (false);
  return 0;
}

void generic_pipe_buf_unmap(struct pipe_inode_info *pipe, struct pipe_buffer *buf, void *address)
{
  sim_assert (false);
}

int generic_pipe_buf_confirm(struct pipe_inode_info *pipe, struct pipe_buffer *buf)
{
	  sim_assert (false);
	  return 0;
}

void generic_pipe_buf_release(struct pipe_inode_info *pipe,
                              struct pipe_buffer *buf)
{
  sim_assert (false);
  return;
}

static int generic_pipe_buf_nosteal(struct pipe_inode_info *pipe,
                                    struct pipe_buffer *buf)
{
  return 1;
}

void generic_pipe_buf_get(struct pipe_inode_info *pipe, struct pipe_buffer *buf)
{
  sim_assert (false);
  return;
}

int proc_nr_inodes(ctl_table *table, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	  sim_assert (false);
	  return 0;
}

int proc_nr_dentry(ctl_table *table, int write, void __user *buffer,
		   size_t *lenp, loff_t *ppos)
{
	sim_assert (false);
		  return 0;
}

/*
 * Handle writeback of dirty data for the device backed by this bdi. Also
 * wakes up periodically and does kupdated style flushing.
 */
int bdi_writeback_thread(void *data)
{
	sim_assert (false);

	return 0;
}

void get_filesystem(struct file_system_type *fs)
{
	return;
}
//#include <fs/proc/proc_sysctl.c>
unsigned int nr_free_buffer_pages(void)
{
	return 1024;
}

const struct pipe_buf_operations nosteal_pipe_buf_ops = {
	.can_merge = 0,
	.confirm = generic_pipe_buf_confirm,
	.release = generic_pipe_buf_release,
	.steal = generic_pipe_buf_nosteal,
	.get = generic_pipe_buf_get,
};

ssize_t
generic_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
        return 0;
}
