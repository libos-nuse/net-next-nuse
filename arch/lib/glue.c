/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 *         Frederic Urbani
 */

#include <linux/types.h>        /* loff_t */
#include <linux/errno.h>        /* ESPIPE */
#include <linux/pagemap.h>      /* PAGE_CACHE_SIZE */
#include <linux/limits.h>       /* NAME_MAX */
#include <linux/statfs.h>       /* struct kstatfs */
#include <linux/bootmem.h>      /* HASHDIST_DEFAULT */
#include <linux/utsname.h>
#include <linux/binfmts.h>
#include <linux/init_task.h>
#include <linux/sched/rt.h>
#include <linux/backing-dev.h>
#include <stdarg.h>
#include "sim-assert.h"
#include "sim.h"
#include "lib.h"


struct pipe_buffer;
struct file;
struct pipe_inode_info;
struct wait_queue_t;
struct kernel_param;
struct super_block;
struct tvec_base {};

/* defined in sched.c, used in net/sched/em_meta.c */
unsigned long avenrun[3];
/* defined in mm/page_alloc.c, used in net/xfrm/xfrm_hash.c */
int hashdist = HASHDIST_DEFAULT;
/* defined in mm/page_alloc.c */
struct pglist_data __refdata contig_page_data;
/* defined in linux/mmzone.h mm/memory.c */
struct page *mem_map = 0;
/* used during boot. */
struct tvec_base boot_tvec_bases;
/* used by sysinfo in kernel/timer.c */
int nr_threads = 0;
/* not very useful in mm/vmstat.c */
atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];

/* XXX: used in network stack ! */
unsigned long num_physpages = 0;
unsigned long totalram_pages = 0;

/* XXX figure out initial value */
unsigned int interrupt_pending = 0;
static unsigned long g_irqflags = 0;
static unsigned long local_irqflags = 0;
int overflowgid = 0;
int overflowuid = 0;
int fs_overflowgid = 0;
int fs_overflowuid = 0;
unsigned long sysctl_overcommit_kbytes __read_mostly;
DEFINE_PER_CPU(struct task_struct *, ksoftirqd);
static DECLARE_BITMAP(cpu_possible_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_possible_mask = to_cpumask(cpu_possible_bits);

struct backing_dev_info noop_backing_dev_info = {
	.name		= "noop",
	.capabilities	= 0,
};

/* from rt.c */
int sched_rr_timeslice = RR_TIMESLICE;
/* from main.c */
bool initcall_debug;
bool static_key_initialized __read_mostly = false;
unsigned long __start_rodata, __end_rodata;

unsigned long arch_local_save_flags(void)
{
	return local_irqflags;
}
void arch_local_irq_restore(unsigned long flags)
{
	local_irqflags = flags;
}


unsigned long long nr_context_switches(void)
{
	/* we just need to return >0 to avoid the warning
	   in kernel/rcupdate.c */
	return 1;
}


long get_user_pages(struct task_struct *tsk, struct mm_struct *mm,
		    unsigned long start, unsigned long nr_pages,
		    int write, int force, struct page **pages,
		    struct vm_area_struct **vmas)
{
	/* in practice, this function is never called. It's linked in because */
	/* we link in get_user_pages_fast which is included only because it */
	/* is located in mm/util.c */
	lib_assert(false);
	return 0;
}


void dump_stack(void)
{
	/* we assert to make sure that we catch whoever calls dump_stack */
	lib_assert(false);
}


void lib_printf(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	lib_vprintf(str, args);
	va_end(args);
}

#include <linux/vmalloc.h>
#include <linux/kmemleak.h>

static unsigned long __meminitdata nr_kernel_pages = 8192;
static unsigned long __meminitdata nr_all_pages = 81920;
/*
 * allocate a large system hash table from bootmem
 * - it is assumed that the hash table must contain an exact power-of-2
 *   quantity of entries
 * - limit is the number of hash buckets, not the total allocation size
 */
void *__init alloc_large_system_hash(const char *tablename,
				     unsigned long bucketsize,
				     unsigned long numentries,
				     int scale,
				     int flags,
				     unsigned int *_hash_shift,
				     unsigned int *_hash_mask,
				     unsigned long low_limit,
				     unsigned long high_limit)
{
	unsigned long long max = high_limit;
	unsigned long log2qty, size;
	void *table = NULL;

	/* allow the kernel cmdline to have a say */
	if (!numentries) {
		/* round applicable memory size up to nearest megabyte */
		numentries = nr_kernel_pages;
		numentries += (1UL << (20 - PAGE_SHIFT)) - 1;
		numentries >>= 20 - PAGE_SHIFT;
		numentries <<= 20 - PAGE_SHIFT;

		/* limit to 1 bucket per 2^scale bytes of low memory */
		if (scale > PAGE_SHIFT)
			numentries >>= (scale - PAGE_SHIFT);
		else
			numentries <<= (PAGE_SHIFT - scale);

		/* Make sure we've got at least a 0-order allocation.. */
		if (unlikely(flags & HASH_SMALL)) {
			/* Makes no sense without HASH_EARLY */
			WARN_ON(!(flags & HASH_EARLY));
			if (!(numentries >> *_hash_shift)) {
				numentries = 1UL << *_hash_shift;
				BUG_ON(!numentries);
			}
		} else if (unlikely((numentries * bucketsize) < PAGE_SIZE))
			numentries = PAGE_SIZE / bucketsize;
	}
	numentries = roundup_pow_of_two(numentries);

	/* limit allocation size to 1/16 total memory by default */
	if (max == 0) {
		max = ((unsigned long long)nr_all_pages << PAGE_SHIFT) >> 4;
		do_div(max, bucketsize);
	}

	if (numentries > max)
		numentries = max;

	log2qty = ilog2(numentries);

	do {
		size = bucketsize << log2qty;
		if (flags & HASH_EARLY)
			table = alloc_bootmem_nopanic(size);
		else if (hashdist)
			table = __vmalloc(size, GFP_ATOMIC, PAGE_KERNEL);
		else {
			/*
			 * If bucketsize is not a power-of-two, we may free
			 * some pages at the end of hash table which
			 * alloc_pages_exact() automatically does
			 */
			if (get_order(size) < MAX_ORDER) {
				table = alloc_pages_exact(size, GFP_ATOMIC);
				kmemleak_alloc(table, size, 1, GFP_ATOMIC);
			}
		}
	} while (!table && size > PAGE_SIZE && --log2qty);

	if (!table)
		panic("Failed to allocate %s hash table\n", tablename);

	pr_info("%s hash table entries: %d (order: %d, %lu bytes)\n",
	       tablename,
	       (1U << log2qty),
	       ilog2(size) - PAGE_SHIFT,
	       size);

	if (_hash_shift)
		*_hash_shift = log2qty;
	if (_hash_mask)
		*_hash_mask = (1 << log2qty) - 1;

	return table;
}

void si_meminfo(struct sysinfo *val)
{
	/* This function is called from the ip layer to get information about
	   the amount of memory in the system and make some educated guesses
	   about some default buffer sizes. We pick a value which ensures
	   small buffers. */
	val->totalram = 0;
}
int slab_is_available(void)
{
	/* called from kernel/param.c. */
	return 1;
}

/* used from kern_ptr_validate from mm/util.c which is never called */
void *high_memory = 0;



void async_synchronize_full(void)
{
	/* called from drivers/base/ *.c */
	/* there is nothing to do, really. */
}

int send_sig(int signal, struct task_struct *task, int x)
{
	struct SimTask *lib_task = container_of(task, struct SimTask,
						kernel_task);

	lib_signal_raised((struct SimTask *)lib_task, signal);
	/* lib_assert (false); */
	return 0;
}
unsigned long get_taint(void)
{
	/* never tainted. */
	return 0;
}
void add_taint(unsigned flag, enum lockdep_ok lockdep_ok)
{
}
struct pid *cad_pid = 0;

void add_device_randomness(const void *buf, unsigned int size)
{
}

int sched_rr_handler(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp,
		     loff_t *ppos)
{
	return 0;
}

int sysctl_max_threads(struct ctl_table *table, int write,
		       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	return 1;
}

void on_each_cpu_mask(const struct cpumask *mask,
		      smp_call_func_t func, void *info, bool wait)
{
}
