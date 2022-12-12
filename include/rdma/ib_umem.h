/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
/*
 * Copyright (c) 2007 Cisco Systems.  All rights reserved.
 */

#ifndef IB_UMEM_H
#define IB_UMEM_H

#include <linux/list.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>
#include <rdma/ib_verbs.h>

struct ib_ucontext;
struct ib_umem_odp;

struct ib_umem {
	struct ib_device       *ibdev;
	struct mm_struct       *owning_mm;
	u64 iova;
	size_t			length;
	unsigned long		address;
	u32 writable : 1;
	u32 is_odp : 1;
	struct work_struct	work;
	struct sg_table sg_head;
	int             nmap;
	unsigned int    sg_nents;
};

/* Returns the offset of the umem start relative to the first page. */
static inline int ib_umem_offset(struct ib_umem *umem)
{
	return umem->address & ~PAGE_MASK;
}

static inline size_t ib_umem_num_dma_blocks(struct ib_umem *umem,
					    unsigned long pgsz)
{
	return (size_t)((ALIGN(umem->iova + umem->length, pgsz) -
			 ALIGN_DOWN(umem->iova, pgsz))) /
	       pgsz;
}

static inline size_t ib_umem_num_pages(struct ib_umem *umem)
{
	return ib_umem_num_dma_blocks(umem, PAGE_SIZE);
}

static inline void __rdma_umem_block_iter_start(struct ib_block_iter *biter,
						struct ib_umem *umem,
						unsigned long pgsz)
{
	__rdma_block_iter_start(biter, umem->sg_head.sgl, umem->nmap, pgsz);
}

/**
 * rdma_umem_for_each_dma_block - iterate over contiguous DMA blocks of the umem
 * @umem: umem to iterate over
 * @pgsz: Page size to split the list into
 *
 * pgsz must be <= PAGE_SIZE or computed by ib_umem_find_best_pgsz(). The
 * returned DMA blocks will be aligned to pgsz and span the range:
 * ALIGN_DOWN(umem->address, pgsz) to ALIGN(umem->address + umem->length, pgsz)
 *
 * Performs exactly ib_umem_num_dma_blocks() iterations.
 */
#define rdma_umem_for_each_dma_block(umem, biter, pgsz)                        \
	for (__rdma_umem_block_iter_start(biter, umem, pgsz);                  \
	     __rdma_block_iter_next(biter);)

#ifdef CONFIG_INFINIBAND_USER_MEM

struct ib_umem *ib_umem_get(struct ib_device *device, unsigned long addr,
			    size_t size, int access);
void ib_umem_release(struct ib_umem *umem);
int ib_umem_copy_from(void *dst, struct ib_umem *umem, size_t offset,
		      size_t length);
unsigned long ib_umem_find_best_pgsz(struct ib_umem *umem,
				     unsigned long pgsz_bitmap,
				     unsigned long virt);

#else /* CONFIG_INFINIBAND_USER_MEM */

#include <linux/err.h>

static inline struct ib_umem *ib_umem_get(struct ib_device *device,
					  unsigned long addr, size_t size,
					  int access)
{
	return ERR_PTR(-EINVAL);
}
static inline void ib_umem_release(struct ib_umem *umem) { }
static inline int ib_umem_copy_from(void *dst, struct ib_umem *umem, size_t offset,
		      		    size_t length) {
	return -EINVAL;
}
static inline unsigned long ib_umem_find_best_pgsz(struct ib_umem *umem,
						   unsigned long pgsz_bitmap,
						   unsigned long virt)
{
	return 0;
}

#endif /* CONFIG_INFINIBAND_USER_MEM */

#endif /* IB_UMEM_H */
