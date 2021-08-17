/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#include <fs/mount.h>

#include "sim-assert.h"

//__cacheline_aligned_in_smp DEFINE_SEQLOCK(mount_lock);
unsigned int dirtytime_expire_interval;

/*void __init mnt_init(void)
{
}


struct vfsmount *kern_mount_data(struct file_system_type *type, void *data)
{
	static struct mount local_mnt;	
	static int count = 0;
	struct mount *mnt = &local_mnt;
	struct dentry *root = 0;

	
	if (count != 0) return &local_mnt.mnt;
	count++;

	memset(mnt, 0, sizeof(struct mount));
	if (!type)
		return ERR_PTR(-ENODEV);
	int flags = SB_KERNMOUNT;
	char *name = (char *)type->name;

	if (flags & SB_KERNMOUNT)
		mnt->mnt.mnt_flags = MNT_INTERNAL;

	root = type->mount(type, flags, name, data);
	if (IS_ERR(root))
		return ERR_CAST(root);

	mnt->mnt.mnt_root = root;
	mnt->mnt.mnt_sb = root->d_sb;
	mnt->mnt_mountpoint = mnt->mnt.mnt_root;
	mnt->mnt_parent = mnt;
	
	list_add_tail(&mnt->mnt_instance, &root->d_sb->s_mounts);

	return &mnt->mnt;
}

struct vfsmount *kern_mount(struct file_system_type *type)
{
	return kern_mount_data(type,NULL);
}*/

void inode_wait_for_writeback(struct inode *inode)
{
}
void truncate_inode_pages_final(struct address_space *mapping)
{
}
int dirtytime_interval_handler(struct ctl_table *table, int write,
			       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	return -ENOSYS;
}

unsigned int nr_free_buffer_pages(void)
{
	return 65535;
}
