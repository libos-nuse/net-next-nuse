/*
 * sysctl wrapper for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/mman.h>
#include <linux/ratelimit.h>
#include <linux/proc_fs.h>
#include "sim-assert.h"
#include "sim-types.h"

int drop_caches_sysctl_handler(struct ctl_table *table, int write,
			       void *buffer, size_t *length, loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int lowmem_reserve_ratio_sysctl_handler(struct ctl_table *table, int write,
					void *buffer, size_t *length,
					loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int min_free_kbytes_sysctl_handler(struct ctl_table *table, int write,
				   void *buffer, size_t *length, loff_t *ppos)
{
	lib_assert(false);
	return 0;
}

int percpu_pagelist_fraction_sysctl_handler(struct ctl_table *table, int write,
					    void *buffer, size_t *length,
					    loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int dirty_background_ratio_handler(struct ctl_table *table, int write,
				   void *buffer, size_t *lenp,
				   loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int dirty_background_bytes_handler(struct ctl_table *table, int write,
				   void *buffer, size_t *lenp,
				   loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int dirty_ratio_handler(struct ctl_table *table, int write,
			void *buffer, size_t *lenp,
			loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int dirty_bytes_handler(struct ctl_table *table, int write,
			void *buffer, size_t *lenp,
			loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int dirty_writeback_centisecs_handler(struct ctl_table *table, int write,
				      void *buffer, size_t *length,
				      loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int scan_unevictable_handler(struct ctl_table *table, int write,
			     void __user *buffer,
			     size_t *length, loff_t *ppos)
{
	lib_assert(false);
	return 0;
}
int sched_rt_handler(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp,
		     loff_t *ppos)
{
	lib_assert(false);
	return 0;
}

//int sysctl_overcommit_memory = OVERCOMMIT_GUESS;
//int sysctl_overcommit_ratio = 50;
int sysctl_panic_on_oom = 0;
int sysctl_oom_dump_tasks = 0;
int sysctl_oom_kill_allocating_task = 0;
int sysctl_nr_trim_pages = 0;
int sysctl_drop_caches = 0;
int sysctl_lowmem_reserve_ratio[MAX_NR_ZONES] = { 32 };
unsigned int sysctl_sched_child_runs_first = 0;
unsigned int sysctl_sched_compat_yield = 0;
unsigned int sysctl_sched_rt_period = 1000000;
int sysctl_sched_rt_runtime = 950000;

int vm_highmem_is_dirtyable;
unsigned long vm_dirty_bytes = 0;
int vm_dirty_ratio = 20;
int dirty_background_ratio = 10;
unsigned int dirty_expire_interval = 30 * 100;
unsigned int dirty_writeback_interval = 5 * 100;
unsigned long dirty_background_bytes = 0;
int percpu_pagelist_fraction = 0;
int panic_timeout = 0;
int panic_on_oops = 0;
int printk_delay_msec = 0;
int panic_on_warn = 0;
DEFINE_RATELIMIT_STATE(printk_ratelimit_state, 5 * HZ, 10);

#define RESERVED_PIDS 300
int pid_max = PID_MAX_DEFAULT;
int pid_max_min = RESERVED_PIDS + 1;
int pid_max_max = PID_MAX_LIMIT;
int min_free_kbytes = 1024;
int max_threads = 100;
int laptop_mode = 0;

#define DEFAULT_MESSAGE_LOGLEVEL 4
#define MINIMUM_CONSOLE_LOGLEVEL 1
#define DEFAULT_CONSOLE_LOGLEVEL 7
int console_printk[4] = {
	DEFAULT_CONSOLE_LOGLEVEL,       /* console_loglevel */
	DEFAULT_MESSAGE_LOGLEVEL,       /* default_message_loglevel */
	MINIMUM_CONSOLE_LOGLEVEL,       /* minimum_console_loglevel */
	DEFAULT_CONSOLE_LOGLEVEL,       /* default_console_loglevel */
};

int print_fatal_signals = 0;
unsigned int core_pipe_limit = 0;
int core_uses_pid = 0;
int vm_swappiness = 60;
int nr_pdflush_threads = 0;
unsigned long scan_unevictable_pages = 0;
int suid_dumpable = 0;
int page_cluster = 0;
int block_dump = 0;
int C_A_D = 0;
#include <linux/nsproxy.h>
struct nsproxy init_nsproxy;
#include <linux/reboot.h>
char poweroff_cmd[POWEROFF_CMD_PATH_LEN] = "/sbin/poweroff";
//unsigned long sysctl_user_reserve_kbytes __read_mostly = 1UL << 17; /* 128MB */
//unsigned long sysctl_admin_reserve_kbytes __read_mostly = 1UL << 13; /* 8MB */

int pdflush_proc_obsolete(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	return nr_pdflush_threads;
}
#include <linux/fs.h>

/**
 * Honestly, I don't understand half of that code.
 * It was modeled after fs/proc/proc_sysctl.c proc_sys_readdir
 *
 * Me either ;) (Hajime, Jan 2013)
 */

/* from proc_sysctl.c (XXX) */
extern struct ctl_table_root sysctl_table_root;
void ctl_table_first_entry(struct ctl_dir *dir,
		 struct ctl_table_header **phead, struct ctl_table **pentry);
void ctl_table_next_entry(struct ctl_table_header **phead, struct ctl_table **pentry);
struct ctl_table *ctl_table_find_entry(struct ctl_table_header **phead,
			     struct ctl_dir *dir, const char *name,
			     int namelen);
struct ctl_dir *ctl_table_xlate_dir(struct ctl_table_set *set, struct ctl_dir *dir);
/* for init_net (XXX, should be fixed) */
#include <net/net_namespace.h>

static void iterate_table_recursive(const struct SimSysIterator *iter,
				    struct ctl_table_header *head)
{

}


static void iterate_recursive(const struct SimSysIterator *iter,
			      struct ctl_table_header *head)
{
	
}


void lib_sys_iterate_files(const struct SimSysIterator *iter)
{

}

int lib_sys_file_read(const struct SimSysFile *file, char *buffer, int size,
		      int offset)
{

}
int lib_sys_file_write(const struct SimSysFile *file, const char *buffer,
		       int size, int offset)
{

}
