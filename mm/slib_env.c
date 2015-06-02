#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/memblock.h>
#include <linux/bootmem.h>
#include <asm/atomic.h>
#include "slib_env.h"

struct meminfo meminfo;

static inline void
free_memmap(unsigned long start_pfn, unsigned long end_pfn)
{
	struct page *start_pg, *end_pg;
	phys_addr_t pg, pgend;

	/*
	 * Convert start_pfn/end_pfn to a struct page pointer.
	 */
	start_pg = pfn_to_page(start_pfn - 1) + 1;
	end_pg = pfn_to_page(end_pfn - 1) + 1;

	/*
	 * Convert to physical addresses, and
	 * round start upwards and end downwards.
	 */
	pg = PAGE_ALIGN(__pa(start_pg));
	pgend = __pa(end_pg) & PAGE_MASK;

	/*
	 * If there are free pages between these,
	 * free the section of the memmap array.
	 */
	if (pg < pgend)
		memblock_free_early(pg, pgend - pg);
}


/*
 * The mem_map array can get very big.  Free the unused area of the memory map.
 */
static void __init free_unused_memmap(void)
{
	unsigned long start, prev_end = 0;
	struct memblock_region *reg;

	/*
	 * This relies on each bank being in address order.
	 * The banks are sorted previously in bootmem_init().
	 */
	for_each_memblock(memory, reg) {
		start = memblock_region_memory_base_pfn(reg);

		/*
		 * Align down here since the VM subsystem insists that the
		 * memmap entries are valid from the bank start aligned to
		 * MAX_ORDER_NR_PAGES.
		 */

		start = round_down(start, MAX_ORDER_NR_PAGES);

		/*
		 * If we had a previous bank, and there is a space
		 * between the current bank and the previous, free it.
		 */
		if (prev_end && prev_end < start)
			free_memmap(prev_end, start);

		/*
		 * Align up here since the VM subsystem insists that the
		 * memmap entries are valid from the bank end aligned to
		 * MAX_ORDER_NR_PAGES.
		 */
		prev_end = ALIGN(memblock_region_memory_end_pfn(reg),
				 MAX_ORDER_NR_PAGES);
	}
}

#ifdef CONFIG_HIGHMEM
static inline void free_area_high(unsigned long pfn, unsigned long end)
{
	for (; pfn < end; pfn++)
		free_highmem_page(pfn_to_page(pfn));
}
#endif

static void __init free_highpages(void)
{
#ifdef CONFIG_HIGHMEM
	unsigned long max_low = max_low_pfn;
	struct memblock_region *mem, *res;

	/* set highmem page free */
	for_each_memblock(memory, mem) {
		unsigned long start = memblock_region_memory_base_pfn(mem);
		unsigned long end = memblock_region_memory_end_pfn(mem);

		/* Ignore complete lowmem entries */
		if (end <= max_low)
			continue;

		/* Truncate partial highmem entries */
		if (start < max_low)
			start = max_low;

		/* Find and exclude any reserved regions */
		for_each_memblock(reserved, res) {
			unsigned long res_start, res_end;

			res_start = memblock_region_reserved_base_pfn(res);
			res_end = memblock_region_reserved_end_pfn(res);

			if (res_end < start)
				continue;
			if (res_start < start)
				res_start = start;
			if (res_start > end)
				res_start = end;
			if (res_end > end)
				res_end = end;
			if (res_start != start)
				free_area_high(start, res_start);
			start = res_end;
			if (start == end)
				break;
		}

		/* And now free anything which remains */
		if (start < end)
			free_area_high(start, end);
	}
#endif
}


/*
 * mem_init() marks the free areas in the mem_map and tells us how much
 * memory is free.  This is done after various parts of the system have
 * claimed their memory after the kernel image.
 */
void __init mem_init(void)
{
	set_max_mapnr(pfn_to_page(max_pfn) - mem_map);

	/* this will put all unused low memory onto the freelists */
	free_unused_memmap();
	free_all_bootmem();

	free_highpages();

	//mem_init_print_info(NULL);

}

static void __init zone_sizes_init(unsigned long min, unsigned long max_low,
	unsigned long max_high)
{
	unsigned long zone_size[MAX_NR_ZONES], zhole_size[MAX_NR_ZONES];
	int i;

	/*
	 * initialise the zones.
	 */
	memset(zone_size, 0, sizeof(zone_size));
	memset(zhole_size, 0, sizeof(zhole_size));

	zone_size[0] = 194560;
	zone_size[1] = 329728;

	free_area_init_node(0, zone_size, min, zhole_size);
}

void __init arm_memblock_init(void)
{
	memblock_dump_all();
}


int __init arm_add_memory(u64 start, u64 size)
{
	u64 aligned_start;

	/*
	 * Ensure that start/size are aligned to a page boundary.
	 * Size is rounded down, start is rounded up.
	 */
	aligned_start = PAGE_ALIGN(start);
	if (aligned_start > start + size)
		size = 0;
	else
		size -= aligned_start - start;

	if (aligned_start < PHYS_OFFSET) {
		if (aligned_start + size <= PHYS_OFFSET) {
			pr_info("Ignoring memory below PHYS_OFFSET1: 0x%08llx-0x%08llx\n",
				aligned_start, aligned_start + size);
			return -EINVAL;
		}

		pr_info("Ignoring memory below PHYS_OFFSET2: 0x%08llx-0x%08llx\n",
			aligned_start, (u64)PHYS_OFFSET);

		size -= PHYS_OFFSET - aligned_start;
		aligned_start = PHYS_OFFSET;
	}

	start = aligned_start;
	size = size & ~(phys_addr_t)(PAGE_SIZE - 1);

	printk("I am %s start:%llu, size:%llu\n", __func__, start, size);

	/*
	 * Check whether this memory region has non-zero size or
	 * invalid node number.
	 */
	if (size == 0)
		return -EINVAL;

	memblock_add(start, size);
	return 0;
}

static void __init find_limits(unsigned long *min, unsigned long *max_low,
			       unsigned long *max_high)
{
	struct meminfo *mi = &meminfo;
	int i;

	/* This assumes the meminfo array is properly sorted */
	*min = bank_pfn_start(&mi->bank[0]);
	for_each_bank (i, mi)
		if (mi->bank[i].highmem)
				break;
	*max_low = bank_pfn_end(&mi->bank[i - 1]);
	*max_high = bank_pfn_end(&mi->bank[mi->nr_banks - 1]);
}

static void __init arm_bootmem_init(unsigned long start_pfn,
	unsigned long end_pfn)
{
	struct memblock_region *reg;
	unsigned int boot_pages;
	phys_addr_t bitmap;
	pg_data_t *pgdat;

	/*
	 * Allocate the bootmem bitmap page.  This must be in a region
	 * of memory which has already been mapped.
	 */
	boot_pages = bootmem_bootmap_pages(end_pfn - start_pfn);

	bitmap = memblock_alloc_base(boot_pages << PAGE_SHIFT, L1_CACHE_BYTES,
				__pfn_to_phys(end_pfn));

	/*
	 * Initialise the bootmem allocator, handing the
	 * memory banks over to bootmem.
	 */
	node_set_online(0);
	pgdat = NODE_DATA(0);
	init_bootmem_node(pgdat, __phys_to_pfn(bitmap), start_pfn, end_pfn);

	/* Free the lowmem regions from memblock into bootmem. */
	for_each_memblock(memory, reg) {
		unsigned long start = memblock_region_memory_base_pfn(reg);
		unsigned long end = memblock_region_memory_end_pfn(reg);

		if (end >= end_pfn)
			end = end_pfn;
		if (start >= end)
			break;

		free_bootmem(__pfn_to_phys(start), (end - start) << PAGE_SHIFT);
	}

	/* Reserve the lowmem memblock reserved regions in bootmem. */
	for_each_memblock(reserved, reg) {
		unsigned long start = memblock_region_reserved_base_pfn(reg);
		unsigned long end = memblock_region_reserved_end_pfn(reg);

		if (end >= end_pfn)
			end = end_pfn;
		if (start >= end)
			break;
		reserve_bootmem(__pfn_to_phys(start),
			        (end - start) << PAGE_SHIFT, BOOTMEM_DEFAULT);
	}
}


void __init bootmem_init(void)
{
	unsigned long min, max_low, max_high;

	find_limits(&min, &max_low, &max_high);

	zone_sizes_init(min, max_low, max_high);

	/*
	 * This doesn't seem to be used by the Linux memory manager any
	 * more, but is used by ll_rw_block.  If we can get rid of it, we
	 * also get rid of some of the stuff above as well.
	 */
	min_low_pfn = min;
	max_low_pfn = max_low;
	max_pfn = max_high;
}


void __init paging_init(void)
{
	bootmem_init();
}


void __init setup_arch(char **cmd)
{
	int ret;
	ret = arm_add_memory(0, 1024 * 1024 * 1024 * 1);
	if (ret)
		printk("arm_add_memory failed in %s\n", __func__);

	arm_memblock_init();
	paging_init();
}


/*
 * Set up kernel memory allocators
 */
static void __init mm_init(void)
{
	mem_init();
}

void __init init_memory_system(void)
{
	setup_arch(NULL);
	page_alloc_init();
	build_all_zonelists(NULL, NULL);
	mm_init();
}

void test(void)
{
	pg_data_t *pgdat = NODE_DATA(nid);
	alloc_pages(GFP_KERNEL, 1);
	
	//printk("I am printk: %p, %p, %d\n", pgdat->node_zones, 
	//					pgdat->node_zonelists, 
	//					pgdat->nr_zones);
}

