#include <linux/seq_file.h>

struct hlist_node *seq_hlist_next_rcu(void *v,
				      struct hlist_head *head,
				      loff_t *ppos)
{
  return 0;
}
struct hlist_node *seq_hlist_start_head_rcu(struct hlist_head *head,
					    loff_t pos)
{
  return 0;
}
struct list_head *seq_list_start(struct list_head *head,
					loff_t pos)
{
  return 0;
}
struct list_head *seq_list_start_head(struct list_head *head,
				      loff_t pos)
{
  return 0;
}

struct list_head *seq_list_next(void *v, struct list_head *head,
				loff_t *ppos)
{
  return 0;
}

void *__seq_open_private(struct file *file, const struct seq_operations *ops, int psize)
{
  return 0;
}

int seq_open_private(struct file *file, const struct seq_operations *ops, int psize)
{
  return 0;
}

int seq_release_private(struct inode *inode, struct file *file)
{
  return 0;
}

loff_t seq_lseek(struct file *file, loff_t offset, int origin)
{
  return 0;
}
int seq_release(struct inode *inode, struct file *file)
{
  return 0;
}
int seq_putc(struct seq_file *m, char c)
{
  return 0;
}
int seq_puts(struct seq_file *m, const char *s)
{
  return 0;
}
ssize_t seq_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
  return 0;
}
int seq_open(struct file *file, const struct seq_operations *ops)
{
  return 0;
}
int seq_printf(struct seq_file *seq, const char *fmt, ...)
{
  return 0;
}

int seq_write(struct seq_file *seq, const void *data, size_t len)
{
  return 0;
}

int seq_open_net(struct inode *inode, struct file *file,
                 const struct seq_operations *ops, int size)
{
  return 0;
}

int seq_release_net(struct inode *inode, struct file *file)
{
  return 0;
}


int single_open(struct file *file, int (*cb)(struct seq_file *, void *), void *data)
{
  return 0;
}

int single_release(struct inode *inode, struct file *file)
{
  return 0;
}

int single_open_net(struct inode *inode, struct file *file,
		    int (*show)(struct seq_file *, void *))
{
  return 0;
}

int single_release_net(struct inode *inode, struct file *file)
{
  return 0;
}
