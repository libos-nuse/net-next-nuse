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
#include <linux/file.h>
#include <linux/sched/sysctl.h>
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

struct mm_struct init_mm;
struct kmem_cache *fs_cachep;

/* defined in linux/mmzone.h mm/memory.c */
struct page *mem_map = 0;
unsigned long max_mapnr;
unsigned long highest_memmap_pfn __read_mostly;

/* vmscan */
unsigned long vm_total_pages;

/* modules.c */
const struct kernel_symbol __start___ksymtab[1];
const struct kernel_symbol __stop___ksymtab[1];
const struct kernel_symbol __start___ksymtab_gpl[1];
const struct kernel_symbol __stop___ksymtab_gpl[1];
const struct kernel_symbol __start___ksymtab_gpl_future[1];
const struct kernel_symbol __stop___ksymtab_gpl_future[1];
const unsigned long __start___kcrctab[1];
const unsigned long __start___kcrctab_gpl[1];
const unsigned long __start___kcrctab_gpl_future[1];

/* used during boot. */
//struct tvec_base boot_tvec_bases;
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
unsigned long sysctl_overcommit_kbytes __read_mostly;
DEFINE_PER_CPU(struct task_struct *, ksoftirqd);

/* memory.c */
unsigned long highest_memmap_pfn __read_mostly;
unsigned long max_mapnr;

/*
 * Randomize the address space (stacks, mmaps, brk, etc.).
 *
 * ( When CONFIG_COMPAT_BRK=y we exclude brk from randomization,
 *   as ancient (libc5 based) binaries can segfault. )
 */
int randomize_va_space __read_mostly =
#ifdef CONFIG_COMPAT_BRK
					1;
#else
					2;
#endif

/* vmscan.c */
unsigned long vm_total_pages;

/* arm/mmu.c */
pgprot_t pgprot_kernel;
int sysctl_max_map_count __read_mostly = DEFAULT_MAX_MAP_COUNT;

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

unsigned long
arch_get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	lib_assert(false);
	return 0;
}

#ifdef CONFIG_HAVE_ARCH_PFN_VALID
int pfn_valid(unsigned long pfn)
{
	return memblock_is_memory(__pfn_to_phys(pfn));
}
#endif
