#include <linux/export.h>
#include <linux/slab.h>
#include <linux/acct.h>
#include <linux/blkdev.h>
#include <linux/mount.h>
#include <linux/security.h>
#include <linux/writeback.h>		/* for the emergency remount stuff */
#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/backing-dev.h>
#include <linux/rculist_bl.h>
#include <linux/cleancache.h>
#include <linux/fsnotify.h>
#include <sim-assert.h>


void put_super(struct super_block *sb);
int grab_super(struct super_block *s);
void destroy_super(struct super_block *s);

int set_anon_super(struct super_block *s, void *data)
{
	  return 0;
}

struct super_block;
LIST_HEAD(super_blocks);

/**
 *	alloc_super	-	create new superblock
 *	@type:	filesystem type superblock should belong to
 *
 *	Allocates and initializes a new &struct super_block.  alloc_super()
 *	returns a pointer new superblock or %NULL if allocation had failed.
 */
static struct super_block *alloc_super(struct file_system_type *type)
{
	struct super_block *s = kzalloc(sizeof(struct super_block),  GFP_USER);
	static const struct super_operations default_op;

	if (s) {
		if (security_sb_alloc(s)) {
			kfree(s);
			s = NULL;
			goto out;
		}
#ifdef CONFIG_SMP
		s->s_files = alloc_percpu(struct list_head);
		if (!s->s_files) {
			security_sb_free(s);
			kfree(s);
			s = NULL;
			goto out;
		} else {
			int i;

			for_each_possible_cpu(i)
				INIT_LIST_HEAD(per_cpu_ptr(s->s_files, i));
		}
#else
		INIT_LIST_HEAD(&s->s_files);
#endif
		s->s_bdi = &default_backing_dev_info;
		INIT_HLIST_NODE(&s->s_instances);
		INIT_HLIST_BL_HEAD(&s->s_anon);
		INIT_LIST_HEAD(&s->s_inodes);
		INIT_LIST_HEAD(&s->s_dentry_lru);
		INIT_LIST_HEAD(&s->s_inode_lru);
		spin_lock_init(&s->s_inode_lru_lock);
		INIT_LIST_HEAD(&s->s_mounts);
		init_rwsem(&s->s_umount);
		/*
		 * sget() can have s_umount recursion.
		 *
		 * When it cannot find a suitable sb, it allocates a new
		 * one (this one), and tries again to find a suitable old
		 * one.
		 *
		 * In case that succeeds, it will acquire the s_umount
		 * lock of the old one. Since these are clearly distrinct
		 * locks, and this object isn't exposed yet, there's no
		 * risk of deadlocks.
		 *
		 * Annotate this by putting this lock in a different
		 * subclass.
		 */
		down_write_nested(&s->s_umount, SINGLE_DEPTH_NESTING);
		s->s_count = 1;
		atomic_set(&s->s_active, 1);
		mutex_init(&s->s_vfs_rename_mutex);
		lockdep_set_class(&s->s_vfs_rename_mutex, &type->s_vfs_rename_key);
		mutex_init(&s->s_dquot.dqio_mutex);
		mutex_init(&s->s_dquot.dqonoff_mutex);
		init_rwsem(&s->s_dquot.dqptr_sem);
		s->s_maxbytes = MAX_NON_LFS;
		s->s_op = &default_op;
		s->s_time_gran = 1000000000;
		s->cleancache_poolid = -1;

		s->s_shrink.seeks = DEFAULT_SEEKS;
		s->s_shrink.shrink = 0; // prune_super;
		s->s_shrink.batch = 1024;
	}
out:
	return s;
}

/**
 *	deactivate_locked_super	-	drop an active reference to superblock
 *	@s: superblock to deactivate
 *
 *	Drops an active reference to superblock, converting it into a temprory
 *	one if there is no other active references left.  In that case we
 *	tell fs driver to shut it down and drop the temporary reference we
 *	had just acquired.
 *
 *	Caller holds exclusive lock on superblock; that lock is released.
 */
void deactivate_locked_super(struct super_block *s)
{
	struct file_system_type *fs = s->s_type;
	if (atomic_dec_and_test(&s->s_active)) {
		cleancache_invalidate_fs(s);
		fs->kill_sb(s);

		/* caches are now gone, we can safely kill the shrinker now */
		unregister_shrinker(&s->s_shrink);

		/*
		 * We need to call rcu_barrier so all the delayed rcu free
		 * inodes are flushed before we release the fs module.
		 */
		rcu_barrier();
		put_filesystem(fs);
		put_super(s);
	} else {
		up_write(&s->s_umount);
	}
}

/* from fs/super.c */
DEFINE_SPINLOCK(sb_lock);
/**
 *	sget	-	find or create a superblock
 *	@type:	filesystem type superblock should belong to
 *	@test:	comparison callback
 *	@set:	setup callback
 *	@data:	argument to each of them
 */
struct super_block *sget(struct file_system_type *type,
			int (*test)(struct super_block *,void *),
			int (*set)(struct super_block *,void *),
			int flags, void *data)
{
	struct super_block *s = NULL;
	struct super_block *old;
	int err;

retry:
	spin_lock(&sb_lock);
	if (test) {
		hlist_for_each_entry(old, &type->fs_supers, s_instances) {
			if (!test(old, data))
				continue;
			if (!grab_super(old))
				goto retry;
			if (s) {
				up_write(&s->s_umount);
				destroy_super(s);
				s = NULL;
			}
			down_write(&old->s_umount);
			if (unlikely(!(old->s_flags & MS_BORN))) {
				deactivate_locked_super(old);
				goto retry;
			}
			return old;
		}
	}
	if (!s) {
		spin_unlock(&sb_lock);
		s = alloc_super(type);
		if (!s)
			return ERR_PTR(-ENOMEM);
		goto retry;
	}

	err = set(s, data);
	if (err) {
		spin_unlock(&sb_lock);
		up_write(&s->s_umount);
		destroy_super(s);
		return ERR_PTR(err);
	}
	s->s_type = type;
	strlcpy(s->s_id, type->name, sizeof(s->s_id));
	list_add_tail(&s->s_list, &super_blocks);
	hlist_add_head(&s->s_instances, &type->fs_supers);
	spin_unlock(&sb_lock);
//	get_filesystem(type);
//	register_shrinker(&s->s_shrink);
	return s;
}

void kill_anon_super(struct super_block *sb)
{

}
