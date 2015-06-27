#include <linux/const.h>
#include <asm/sections.h>


/* From arm/include/asm/memory.h */
#define __phys_to_pfn(paddr)    ((unsigned long)((paddr) >> PAGE_SHIFT))
#define __pfn_to_phys(pfn)      ((phys_addr_t)(pfn) << PAGE_SHIFT)



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

#define for_each_bank(iter,mi)                          \
        for (iter = 0; iter < (mi)->nr_banks; iter++)

#define bank_pfn_start(bank)    __phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)      __phys_to_pfn((bank)->start + (bank)->size)
#define bank_pfn_size(bank)     ((bank)->size >> PAGE_SHIFT)
#define bank_phys_start(bank)   (bank)->start
#define bank_phys_end(bank)     ((bank)->start + (bank)->size)
#define bank_phys_size(bank)    (bank)->size



/* From arm/mm/mmu.c */
//pgprot_t pgprot_user;
//pgprot_t pgprot_kernel;
//pgprot_t pgprot_hyp_device;
//pgprot_t pgprot_s2;
//pgprot_t pgprot_s2_device;


void __init init_memory_system(void);


