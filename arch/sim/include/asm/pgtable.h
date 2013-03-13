#ifndef _ASM_SIM_PGTABLE_H
#define _ASM_SIM_PGTABLE_H

#define PAGE_KERNEL ((pgprot_t){0})

#define arch_start_context_switch(prev) do {} while (0)

#define kern_addr_valid(addr)(1)
#define pte_file(pte)(1)
#define __swp_entry(type, offset) \
	((swp_entry_t) { ((type) << 5) | ((offset) << 11) })
#define __pte_to_swp_entry(pte)		((swp_entry_t) { pte_val((pte)) })
#define __swp_entry_to_pte(x)		((pte_t) { (x).val })

#endif /* _ASM_SIM_PGTABLE_H */
