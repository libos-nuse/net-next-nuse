#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include "sim-assert.h"

static struct kmem_cache *dentry_cache __read_mostly;
struct kmem_cache *names_cachep __read_mostly;

/**
 * __d_alloc	-	allocate a dcache entry
 * @sb: filesystem it will belong to
 * @name: qstr of the name
 *
 * Allocates a dentry. It returns %NULL if there is insufficient memory
 * available. On a success the dentry is returned. The name passed in is
 * copied and the copy passed in may be reused after this call.
 */
struct dentry *__d_alloc(struct super_block *sb, const struct qstr *name)
{
	struct dentry *dentry;
	unsigned char *dname;

	dentry = kmalloc(sizeof (struct dentry), GFP_KERNEL); //  kmem_cache_alloc(dentry_cache, GFP_KERNEL);
	if (!dentry)
		return NULL;

	if (name->len > DNAME_INLINE_LEN-1) {
		dname = kmalloc(name->len + 1, GFP_KERNEL);
		if (!dname) {
			kmem_cache_free(dentry_cache, dentry);
			return NULL;
		}
	} else  {
		dname = dentry->d_iname;
	}
	dentry->d_name.name = dname;

	dentry->d_name.len = name->len;
	dentry->d_name.hash = name->hash;
	memcpy(dname, name->name, name->len);
	dname[name->len] = 0;

	dentry->d_count = 1;
	dentry->d_flags = 0;
	spin_lock_init(&dentry->d_lock);
	seqcount_init(&dentry->d_seq);
	dentry->d_inode = NULL;
	dentry->d_parent = dentry;
	dentry->d_sb = sb;
	dentry->d_op = NULL;
	dentry->d_fsdata = NULL;
	INIT_HLIST_BL_NODE(&dentry->d_hash);
	INIT_LIST_HEAD(&dentry->d_lru);
	INIT_LIST_HEAD(&dentry->d_subdirs);
	INIT_HLIST_NODE(&dentry->d_alias);
	INIT_LIST_HEAD(&dentry->d_u.d_child);
	d_set_d_op(dentry, dentry->d_sb->s_d_op);

//	this_cpu_inc(nr_dentry);

	return dentry;
}

void d_set_d_op(struct dentry *dentry, const struct dentry_operations *op)
{
	WARN_ON_ONCE(dentry->d_op);
	WARN_ON_ONCE(dentry->d_flags & (DCACHE_OP_HASH	|
				DCACHE_OP_COMPARE	|
				DCACHE_OP_REVALIDATE	|
				DCACHE_OP_DELETE ));
	dentry->d_op = op;
	if (!op)
		return;
	if (op->d_hash)
		dentry->d_flags |= DCACHE_OP_HASH;
	if (op->d_compare)
		dentry->d_flags |= DCACHE_OP_COMPARE;
	if (op->d_revalidate)
		dentry->d_flags |= DCACHE_OP_REVALIDATE;
	if (op->d_delete)
		dentry->d_flags |= DCACHE_OP_DELETE;
	if (op->d_prune)
		dentry->d_flags |= DCACHE_OP_PRUNE;

}
