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
	proc_root_sim->next = 0;
	proc_root_sim->subdir = 0;
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
static struct proc_dir_entry *
proc_create_entry(const char *name,
		  mode_t mode,
		  struct proc_dir_entry *parent)
{
	/* insert new entry at head of subdir list in parent */
	struct proc_dir_entry *child = kzalloc(sizeof(struct proc_dir_entry)
					       + strlen(name) + 1
					       , GFP_KERNEL);

	if (!child)
		return NULL;
	strcpy(child->name, name);
	child->namelen = strlen(name);
	child->parent = parent;
	if (parent == 0)
		parent = proc_root_sim;
	child->next = parent->subdir;
	child->mode = mode;
	parent->subdir = child;
	return child;
}
struct proc_dir_entry *proc_create_data(const char *name, umode_t mode,
					struct proc_dir_entry *parent,
					const struct file_operations *proc_fops,
					void *data)
{
	struct proc_dir_entry *de = proc_create_entry(name, mode, parent);

	de->proc_fops = proc_fops;
	de->data = data;
	return de;
}
void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
	struct proc_dir_entry *de, **prev;

	for (de = parent->subdir, prev = &parent->subdir; de;
	     prev = &de->next, de = de->next) {
		if (strcmp(name, de->name) == 0) {
			*prev = de->next;
			kfree(de->name);
			kfree(de);
			break;
		}
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
	return proc_create_entry(name, S_IRUGO | S_IXUGO, parent);
}

struct proc_dir_entry *proc_mkdir_data(const char *name, umode_t mode,
				       struct proc_dir_entry *parent,
				       void *data)
{
	struct proc_dir_entry *de = proc_create_entry(name, mode, parent);

	de->data = data;
	return de;
}

int proc_alloc_inum(unsigned int *inum)
{
	*inum = 1;
	return 0;
}
