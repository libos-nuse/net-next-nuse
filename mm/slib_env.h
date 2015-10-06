/*
 * Library Slab Allocator (SLIB)
 *
 * Copyright (c) 2015 Yizheng Jiao
 *
 * Author: Yizheng Jiao <jyizheng@gmail.com>
 */

#ifndef SLIB_ENV_H
#define SLIB_ENV_H

#include <linux/const.h>
#include <asm/sections.h>

/*
 * Memory map description: from arm/include/asm/setup.h
 */
#ifdef CONFIG_ARM_NR_BANKS
#define NR_BANKS        CONFIG_ARM_NR_BANKS
#else
#define NR_BANKS        16
#endif

struct membank {
	phys_addr_t start;
	unsigned long size;
	unsigned int highmem;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_BANKS];
};

extern struct meminfo meminfo;

#define for_each_bank(iter, mi)				\
	for (iter = 0; iter < (mi)->nr_banks; iter++)

#define bank_pfn_start(bank)    __phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)      __phys_to_pfn((bank)->start + (bank)->size)
#define bank_pfn_size(bank)     ((bank)->size >> PAGE_SHIFT)
#define bank_phys_start(bank)   ((bank)->start)
#define bank_phys_end(bank)     ((bank)->start + (bank)->size)
#define bank_phys_size(bank)    ((bank)->size)

void __init init_memory_system(void);
extern char *total_ram;

#endif
