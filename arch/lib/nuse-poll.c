#include <linux/poll.h>
#include <linux/net.h>
#include "nuse.h"

extern struct nuse_fd nuse_fd_table[1024];


struct poll_table_page {
	struct poll_table_page *next;
	struct poll_table_entry *entry;
	struct poll_table_entry entries[0];
};

#define POLL_TABLE_FULL(table) \
	((unsigned long)((table)->entry + \
			 1) > PAGE_SIZE + (unsigned long)(table))

static struct poll_table_entry *poll_get_entry(struct poll_wqueues *p)
{
	struct poll_table_page *table = p->table;

	if (p->inline_index < N_INLINE_POLL_ENTRIES)
		return p->inline_entries + p->inline_index++;

	if (!table || POLL_TABLE_FULL(table)) {
		struct poll_table_page *new_table;

		new_table =
			(struct poll_table_page *)__get_free_page(GFP_KERNEL);
		if (!new_table) {
			p->error = -ENOMEM;
			return NULL;
		}
		new_table->entry = new_table->entries;
		new_table->next = table;
		p->table = new_table;
		table = new_table;
	}

	return table->entry++;
}

static int __pollwake(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	struct poll_wqueues *pwq = wait->private;

	DECLARE_WAITQUEUE(dummy_wait, pwq->polling_task);

	/*
	 * Although this function is called under waitqueue lock, LOCK
	 * doesn't imply write barrier and the users expect write
	 * barrier semantics on wakeup functions.  The following
	 * smp_wmb() is equivalent to smp_wmb() in try_to_wake_up()
	 * and is paired with set_mb() in poll_schedule_timeout.
	 */
	smp_wmb();
	pwq->triggered = 1;

	/*
	 * Perform the default wake up operation using a dummy
	 * waitqueue.
	 *
	 * TODO: This is hacky but there currently is no interface to
	 * pass in @sync.  @sync is scheduled to be removed and once
	 * that happens, wake_up_process() can be used directly.
	 */
	return default_wake_function(&dummy_wait, mode, sync, key);
}

static int pollwake(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	struct poll_table_entry *entry;

	entry = container_of(wait, struct poll_table_entry, wait);
	if (key && !((unsigned long)key & entry->key))
		return 0;
	return __pollwake(wait, mode, sync, key);
}

static void __pollwait(struct file *filp, wait_queue_head_t *wait_address,
		       poll_table *p)
{
	struct poll_wqueues *pwq = container_of(p, struct poll_wqueues, pt);
	struct poll_table_entry *entry = poll_get_entry(pwq);

	if (!entry)
		return;
	entry->wait_address = wait_address;
	entry->key = p->_key;
	init_waitqueue_func_entry(&entry->wait, pollwake);
	entry->wait.private = pwq;
	add_wait_queue(wait_address, &entry->wait);
}

void poll_initwait(struct poll_wqueues *pwq)
{
	init_poll_funcptr(&pwq->pt, __pollwait);
	pwq->polling_task = current;
	pwq->triggered = 0;
	pwq->error = 0;
	pwq->table = NULL;
	pwq->inline_index = 0;
}

static void free_poll_entry(struct poll_table_entry *entry)
{
	remove_wait_queue(entry->wait_address, &entry->wait);
}

void poll_freewait(struct poll_wqueues *pwq)
{
	struct poll_table_page *p = pwq->table;
	int i;

	for (i = 0; i < pwq->inline_index; i++)
		free_poll_entry(pwq->inline_entries + i);
	while (p) {
		struct poll_table_entry *entry;
		struct poll_table_page *old;

		entry = p->entry;
		do {
			entry--;
			free_poll_entry(entry);
		} while (entry > p->entries);
		old = p;
		p = p->next;
		free_page((unsigned long)old);
	}
}

static int
do_poll(struct pollfd *fds, unsigned int nfds,
	struct poll_wqueues *wait, struct timespec *end_time)
{
	poll_table *pt = &wait->pt;
	int timed_out = 0;
	int i;
	int count = 0;
	int mask;

	/* Optimise the no-wait case */
	if (end_time && !end_time->tv_sec && !end_time->tv_nsec) {
		pt->_qproc = NULL;
		timed_out = 1;
	}

	/* initialize all outgoing events. */
	for (i = 0; i < nfds; ++i)
		fds[i].revents = 0;


	/* call (sim) kernel side */
	for (i = 0; i < nfds; ++i) {
		struct socket *sock =
			(struct socket *)nuse_fd_table[fds[i].fd].kern_sock;
		struct file zero;

		if (!sock)
			continue;

		pt->_key = fds[i].events | POLLERR | POLLHUP;
		mask = sock->ops->poll(&zero, sock, pt);
		mask &= (fds[i].events | POLLERR | POLLHUP);
		fds[i].revents = mask;
		if (mask) {
			count++;
			pt->_qproc = NULL;
		}
	}

	if (count || timed_out)
		goto end;

	if (!schedule_timeout(end_time ?
			      (timespec_to_jiffies(end_time) -
			       jiffies) : MAX_SCHEDULE_TIMEOUT))
		timed_out = 1;

end:
	return count;
}

int
nuse_poll(struct pollfd *fds, unsigned int nfds, struct timespec *end_time)
{
	struct poll_wqueues table;
	int count = 0;

	poll_initwait(&table);
	count = do_poll(fds, nfds, &table, end_time);
	poll_freewait(&table);

	return count;
}
