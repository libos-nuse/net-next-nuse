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
//#include <linux/bootmem.h>      /* HASHDIST_DEFAULT */
#include <linux/memblock.h>
#include <linux/utsname.h>
#include <linux/binfmts.h>
#include <linux/init_task.h>
#include <linux/sched/rt.h>
#include <linux/backing-dev.h>
#include <trace/events/mmflags.h>
#include <linux/cpuhotplug.h>
#include <stdarg.h>
#include "sim-assert.h"
#include "sim.h"
#include "lib.h"


struct path def_root;
struct mnt_namespace *def_mnt_ns;

struct pipe_buffer;
struct file;
struct pipe_inode_info;
struct wait_queue_t;
struct kernel_param;
struct super_block;

/* defined in sched.c, used in net/sched/em_meta.c */
unsigned long avenrun[3];
/* defined in mm/page_alloc.c */
struct pglist_data __refdata contig_page_data;
/* defined in linux/mmzone.h mm/memory.c */
struct page *mem_map = 0;
/* used by sysinfo in kernel/timer.c */
int nr_threads = 0;
/* not very useful in mm/vmstat.c */
atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];

/* XXX: used in network stack ! */
unsigned long num_physpages = 0;
atomic_long_t _totalram_pages = {
	.counter = 0
};

static unsigned long __init arch_reserved_kernel_pages(void)
{
	return 0;
}

/* XXX figure out initial value */
unsigned int interrupt_pending = 0;
static unsigned long g_irqflags = 0;
static unsigned long local_irqflags = 0;
int overflowgid = 0;
int overflowuid = 0;
int fs_overflowgid = 0;
int fs_overflowuid = 0;
//unsigned long sysctl_overcommit_kbytes __read_mostly;
DEFINE_PER_CPU(struct task_struct *, ksoftirqd);

struct backing_dev_info noop_backing_dev_info = {
	.dev_name		= "noop",
	.capabilities	= 0,
};

/* from rt.c */
int sched_rr_timeslice = RR_TIMESLICE;
/* from main.c */
bool initcall_debug;
bool static_key_initialized __read_mostly = false;
char __start_rodata[], __end_rodata[];

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


long get_user_pages(unsigned long start, unsigned long nr_pages,
			    unsigned int gup_flags, struct page **pages,
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

#define ADAPT_SCALE_BASE	(64ul << 30)
#define ADAPT_SCALE_SHIFT	2
#define ADAPT_SCALE_NPAGES	(ADAPT_SCALE_BASE >> PAGE_SHIFT)

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
	gfp_t gfp_flags;
	bool virt;

	/* allow the kernel cmdline to have a say */
	if (!numentries) {
		/* round applicable memory size up to nearest megabyte */
		numentries = nr_kernel_pages;
		numentries -= arch_reserved_kernel_pages();

		/* It isn't necessary when PAGE_SIZE >= 1MB */
		if (PAGE_SHIFT < 20)
			numentries = round_up(numentries, (1<<20)/PAGE_SIZE);

#if __BITS_PER_LONG > 32
		if (!high_limit) {
			unsigned long adapt;

			for (adapt = ADAPT_SCALE_NPAGES; adapt < numentries;
			     adapt <<= ADAPT_SCALE_SHIFT)
				scale++;
		}
#endif

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
	max = min(max, 0x80000000ULL);

	if (numentries < low_limit)
		numentries = low_limit;
	if (numentries > max)
		numentries = max;

	log2qty = ilog2(numentries);

	gfp_flags = (flags & HASH_ZERO) ? GFP_ATOMIC | __GFP_ZERO : GFP_ATOMIC;
	do {
		virt = false;
		size = bucketsize << log2qty;
		if (flags & HASH_EARLY) {
			if (flags & HASH_ZERO)
				table = memblock_alloc(size, SMP_CACHE_BYTES);
			else
				table = memblock_alloc_raw(size,
							   SMP_CACHE_BYTES);
		} else if (get_order(size) >= MAX_ORDER || hashdist) {
			table = __vmalloc(size, gfp_flags);
			virt = true;
		} else {
			/*
			 * If bucketsize is not a power-of-two, we may free
			 * some pages at the end of hash table which
			 * alloc_pages_exact() automatically does
			 */
			table = alloc_pages_exact(size, gfp_flags);
			kmemleak_alloc(table, size, 1, gfp_flags);
		}
	} while (!table && size > PAGE_SIZE && --log2qty);

	if (!table)
		panic("Failed to allocate %s hash table\n", tablename);

	pr_info("%s hash table entries: %ld (order: %d, %lu bytes, %s)\n",
		tablename, 1UL << log2qty, ilog2(size) - PAGE_SHIFT, size,
		virt ? "vmalloc" : "linear");

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
bool slab_is_available(void)
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

// mm/page-writeback.c
int __set_page_dirty_no_writeback(struct page *page)
{
	if (!PageDirty(page))
		return !TestSetPageDirty(page);
	return 0;
}


// lib/uuid.c
const u8 guid_index[16] = {3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
const u8 uuid_index[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

// fs/ramfs/file-nommu.c
static unsigned long ramfs_mmu_get_unmapped_area(struct file *file,
		unsigned long addr, unsigned long len, unsigned long pgoff,
		unsigned long flags)
{
	return 0;
}
const struct file_operations ramfs_file_operations = {
	.read_iter	= generic_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
	.get_unmapped_area	= ramfs_mmu_get_unmapped_area,
};

const struct inode_operations ramfs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

// fs/nommu.c
const struct vm_operations_struct generic_file_vm_ops = {
};

// mm/vmstat.c
atomic_long_t vm_zone_stat[NR_VM_ZONE_STAT_ITEMS] __cacheline_aligned_in_smp;
atomic_long_t vm_numa_stat[NR_VM_NUMA_STAT_ITEMS] __cacheline_aligned_in_smp;
atomic_long_t vm_node_stat[NR_VM_NODE_STAT_ITEMS] __cacheline_aligned_in_smp;


// mm/debug.c
const struct trace_print_flags pageflag_names[] = {
	__def_pageflag_names,
	{0, NULL}
};
const struct trace_print_flags gfpflag_names[] = {
	__def_gfpflag_names,
	{0, NULL}
};
const struct trace_print_flags vmaflag_names[] = {
	__def_vmaflag_names,
	{0, NULL}
};


// lib/random32.c
DEFINE_PER_CPU(unsigned long, net_rand_noise);

// kernel/capability.c
const kernel_cap_t __cap_empty_set = CAP_EMPTY_SET;

// init/main.c
char *saved_command_line;


// include/linux/oom.h
struct mutex oom_lock;
struct mutex oom_adj_mutex;

// mm/page_alloc.h
int watermark_scale_factor_sysctl_handler(struct ctl_table *table, int write,
		void *buffer, size_t *length, loff_t *ppos)
{
	return 0;
}

// kernel/sched/deadline.c
unsigned int sysctl_sched_dl_period_max = 1 << 22; /* ~4 seconds */
unsigned int sysctl_sched_dl_period_min = 100;     /* 100 us */

// kernel/sched/rt.c
int sysctl_sched_rr_timeslice = (MSEC_PER_SEC / HZ) * RR_TIMESLICE;

// kernel/panic.c
unsigned long panic_print;

// kernel/printk/printk.c
char devkmsg_log_str[DEVKMSG_STR_MAX_SIZE] = "ratelimit";
int devkmsg_sysctl_set_loglvl(struct ctl_table *table, int write,
			      void *buffer, size_t *lenp, loff_t *ppos)
{
	return 0;
}

// kernel/cpu.c
int __cpuhp_setup_state(enum cpuhp_state state,
			const char *name, bool invoke,
			int (*startup)(unsigned int cpu),
			int (*teardown)(unsigned int cpu),
			bool multi_instance)
{
	return 0;
}

//  mm/vmscan.c
int prealloc_shrinker(struct shrinker *shrinker)
{
	unsigned int size = sizeof(*shrinker->nr_deferred);

	if (shrinker->flags & SHRINKER_NUMA_AWARE)
		size *= nr_node_ids;

	shrinker->nr_deferred = kzalloc(size, GFP_KERNEL);
	if (!shrinker->nr_deferred)
	{		
		return -ENOMEM;
	}

	return 0;
}

void register_shrinker_prepared(struct shrinker *shrinker)
{
}
