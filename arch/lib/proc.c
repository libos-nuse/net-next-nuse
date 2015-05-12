/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2014 Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

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

