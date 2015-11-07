#ifndef _ASM_SIM_PGTABLE_H
#define _ASM_SIM_PGTABLE_H

#define PAGE_KERNEL ((pgprot_t) {0 })

#define arch_start_context_switch(prev) do {} while (0)

#define kern_addr_valid(addr)(1)
#define pte_file(pte)(1)
/* Encode and de-code a swap entry */
#define __swp_type(x)                   (((x).val >> 5) & 0x1f)
#define __swp_offset(x)                 ((x).val >> 11)
#define __swp_entry(type, offset) \
	((swp_entry_t) {((type) << 5) | ((offset) << 11) })
#define __pte_to_swp_entry(pte)         ((swp_entry_t) {pte_val((pte)) })
#define __swp_entry_to_pte(x)           ((pte_t) {(x).val })
#define pmd_page(pmd) (struct page *)(pmd_val(pmd) & PAGE_MASK)
#define pgtable_cache_init()   do { } while (0)

static inline int pte_swp_soft_dirty(pte_t pte)
{
	return 0;
}

static inline pte_t pte_swp_clear_soft_dirty(pte_t pte)
{
	return pte;
}

#endif /* _ASM_SIM_PGTABLE_H */
