#include <linux/fs.h>
#include <linux/net.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <net/net_namespace.h>
#include "sim-types.h"
#include "sim-assert.h"
#include "fs/proc/internal.h"           /* XXX */

struct proc_dir_entry;
static char proc_root_data[sizeof(struct proc_dir_entry) + 4];

static struct proc_dir_entry *proc_root_sim  =
	(struct proc_dir_entry *)proc_root_data;

void lib_proc_net_initialize(void)
{
	proc_root_sim->parent = proc_root_sim;
	strcpy(proc_root_sim->name, "net");
	proc_root_sim->mode = S_IFDIR | S_IRUGO | S_IXUGO;
	proc_root_sim->subdir = RB_ROOT;
	init_net.proc_net = proc_root_sim;
	init_net.proc_net_stat = proc_mkdir("stat", proc_root_sim);
}
struct proc_dir_entry *
proc_net_fops_create(struct net *net,
		     const char *name,
		     umode_t mode, const struct file_operations *fops)
{
	return proc_create_data(name, mode, net->proc_net, fops, 0);
}

static struct proc_dir_entry *__proc_create(struct proc_dir_entry **parent,
					  const char *name,
					  umode_t mode,
					  nlink_t nlink)
{
	struct proc_dir_entry *ent = NULL;
	const char *fn = name;
	struct qstr qstr;

	qstr.name = fn;
	qstr.len = strlen(fn);
	if (qstr.len == 0 || qstr.len >= 256) {
		WARN(1, "name len %u\n", qstr.len);
		return NULL;
	}

	ent = kzalloc(sizeof(struct proc_dir_entry) + qstr.len + 1, GFP_KERNEL);
	if (!ent)
		goto out;

	memcpy(ent->name, fn, qstr.len + 1);
	ent->namelen = qstr.len;
	ent->mode = mode;
	ent->nlink = nlink;
	ent->subdir = RB_ROOT;
	atomic_set(&ent->count, 1);
	spin_lock_init(&ent->pde_unload_lock);
	INIT_LIST_HEAD(&ent->pde_openers);
out:
	return ent;
}
struct proc_dir_entry *proc_create_data(const char *name, umode_t mode,
					struct proc_dir_entry *parent,
					const struct file_operations *proc_fops,
					void *data)
{
	struct proc_dir_entry *de;

	de = __proc_create(&parent, name, S_IFDIR | mode, 2);
	de->proc_fops = proc_fops;
	de->data = data;
	return de;
}
static int proc_match(unsigned int len, const char *name, struct proc_dir_entry *de)
{
	if (len < de->namelen)
		return -1;
	if (len > de->namelen)
		return 1;

	return memcmp(name, de->name, len);
}
static struct proc_dir_entry *pde_subdir_find(struct proc_dir_entry *dir,
					      const char *name,
					      unsigned int len)
{
	struct rb_node *node = dir->subdir.rb_node;

	while (node) {
		struct proc_dir_entry *de = container_of(node,
							 struct proc_dir_entry,
							 subdir_node);
		int result = proc_match(len, name, de);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return de;
	}
	return NULL;
}

void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
	struct proc_dir_entry *de, **prev;

	de = pde_subdir_find(parent, name, strlen(name));
	if (de) {
		rb_erase(&de->subdir_node, &parent->subdir);
		kfree(de->name);
		kfree(de);
	}
}
void proc_net_remove(struct net *net, const char *name)
{
	remove_proc_entry(name, net->proc_net);
}
void proc_remove(struct proc_dir_entry *de)
{
	/* XXX */
}
int proc_nr_files(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	return -ENOSYS;
}

struct proc_dir_entry *proc_mkdir(const char *name,
				  struct proc_dir_entry *parent)
{
	return __proc_create(&parent, name, S_IFDIR | S_IRUGO, 2);
}

struct proc_dir_entry *proc_mkdir_data(const char *name, umode_t mode,
				       struct proc_dir_entry *parent,
				       void *data)
{
	struct proc_dir_entry *de;

	de = __proc_create(&parent, name,
			S_IFDIR | S_IRUGO | S_IXUGO | mode, 2);
	de->data = data;
	return de;
}

int proc_alloc_inum(unsigned int *inum)
{
	*inum = 1;
	return 0;
}
