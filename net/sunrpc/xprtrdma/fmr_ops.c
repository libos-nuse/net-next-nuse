/*
 * Copyright (c) 2015 Oracle.  All rights reserved.
 * Copyright (c) 2003-2007 Network Appliance, Inc. All rights reserved.
 */

/* Lightweight memory registration using Fast Memory Regions (FMR).
 * Referred to sometimes as MTHCAFMR mode.
 *
 * FMR uses synchronous memory registration and deregistration.
 * FMR registration is known to be fast, but FMR deregistration
 * can take tens of usecs to complete.
 */

/* Normal operation
 *
 * A Memory Region is prepared for RDMA READ or WRITE using the
 * ib_map_phys_fmr verb (fmr_op_map). When the RDMA operation is
 * finished, the Memory Region is unmapped using the ib_unmap_fmr
 * verb (fmr_op_unmap).
 */

/* Transport recovery
 *
 * After a transport reconnect, fmr_op_map re-uses the MR already
 * allocated for the RPC, but generates a fresh rkey then maps the
 * MR again. This process is synchronous.
 */

#include "xprt_rdma.h"

#if IS_ENABLED(CONFIG_SUNRPC_DEBUG)
# define RPCDBG_FACILITY	RPCDBG_TRANS
#endif

/* Maximum scatter/gather per FMR */
#define RPCRDMA_MAX_FMR_SGES	(64)

static struct workqueue_struct *fmr_recovery_wq;

#define FMR_RECOVERY_WQ_FLAGS		(WQ_UNBOUND)

int
fmr_alloc_recovery_wq(void)
{
	fmr_recovery_wq = alloc_workqueue("fmr_recovery", WQ_UNBOUND, 0);
	return !fmr_recovery_wq ? -ENOMEM : 0;
}

void
fmr_destroy_recovery_wq(void)
{
	struct workqueue_struct *wq;

	if (!fmr_recovery_wq)
		return;

	wq = fmr_recovery_wq;
	fmr_recovery_wq = NULL;
	destroy_workqueue(wq);
}

static int
__fmr_unmap(struct rpcrdma_mw *mw)
{
	LIST_HEAD(l);

	list_add(&mw->fmr.fmr->list, &l);
	return ib_unmap_fmr(&l);
}

/* Deferred reset of a single FMR. Generate a fresh rkey by
 * replacing the MR. There's no recovery if this fails.
 */
static void
__fmr_recovery_worker(struct work_struct *work)
{
	struct rpcrdma_mw *mw = container_of(work, struct rpcrdma_mw,
					    mw_work);
	struct rpcrdma_xprt *r_xprt = mw->mw_xprt;

	__fmr_unmap(mw);
	rpcrdma_put_mw(r_xprt, mw);
	return;
}

/* A broken MR was discovered in a context that can't sleep.
 * Defer recovery to the recovery worker.
 */
static void
__fmr_queue_recovery(struct rpcrdma_mw *mw)
{
	INIT_WORK(&mw->mw_work, __fmr_recovery_worker);
	queue_work(fmr_recovery_wq, &mw->mw_work);
}

static int
fmr_op_open(struct rpcrdma_ia *ia, struct rpcrdma_ep *ep,
	    struct rpcrdma_create_data_internal *cdata)
{
	rpcrdma_set_max_header_sizes(ia, cdata, max_t(unsigned int, 1,
						      RPCRDMA_MAX_DATA_SEGS /
						      RPCRDMA_MAX_FMR_SGES));
	return 0;
}

/* FMR mode conveys up to 64 pages of payload per chunk segment.
 */
static size_t
fmr_op_maxpages(struct rpcrdma_xprt *r_xprt)
{
	return min_t(unsigned int, RPCRDMA_MAX_DATA_SEGS,
		     RPCRDMA_MAX_HDR_SEGS * RPCRDMA_MAX_FMR_SGES);
}

static int
fmr_op_init(struct rpcrdma_xprt *r_xprt)
{
	struct rpcrdma_buffer *buf = &r_xprt->rx_buf;
	int mr_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ;
	struct ib_fmr_attr fmr_attr = {
		.max_pages	= RPCRDMA_MAX_FMR_SGES,
		.max_maps	= 1,
		.page_shift	= PAGE_SHIFT
	};
	struct ib_pd *pd = r_xprt->rx_ia.ri_pd;
	struct rpcrdma_mw *r;
	int i, rc;

	spin_lock_init(&buf->rb_mwlock);
	INIT_LIST_HEAD(&buf->rb_mws);
	INIT_LIST_HEAD(&buf->rb_all);

	i = max_t(int, RPCRDMA_MAX_DATA_SEGS / RPCRDMA_MAX_FMR_SGES, 1);
	i += 2;				/* head + tail */
	i *= buf->rb_max_requests;	/* one set for each RPC slot */
	dprintk("RPC:       %s: initalizing %d FMRs\n", __func__, i);

	rc = -ENOMEM;
	while (i--) {
		r = kzalloc(sizeof(*r), GFP_KERNEL);
		if (!r)
			goto out;

		r->fmr.physaddrs = kmalloc(RPCRDMA_MAX_FMR_SGES *
					   sizeof(u64), GFP_KERNEL);
		if (!r->fmr.physaddrs)
			goto out_free;

		r->fmr.fmr = ib_alloc_fmr(pd, mr_access_flags, &fmr_attr);
		if (IS_ERR(r->fmr.fmr))
			goto out_fmr_err;

		r->mw_xprt = r_xprt;
		list_add(&r->mw_list, &buf->rb_mws);
		list_add(&r->mw_all, &buf->rb_all);
	}
	return 0;

out_fmr_err:
	rc = PTR_ERR(r->fmr.fmr);
	dprintk("RPC:       %s: ib_alloc_fmr status %i\n", __func__, rc);
	kfree(r->fmr.physaddrs);
out_free:
	kfree(r);
out:
	return rc;
}

/* Use the ib_map_phys_fmr() verb to register a memory region
 * for remote access via RDMA READ or RDMA WRITE.
 */
static int
fmr_op_map(struct rpcrdma_xprt *r_xprt, struct rpcrdma_mr_seg *seg,
	   int nsegs, bool writing)
{
	struct rpcrdma_ia *ia = &r_xprt->rx_ia;
	struct ib_device *device = ia->ri_device;
	enum dma_data_direction direction = rpcrdma_data_dir(writing);
	struct rpcrdma_mr_seg *seg1 = seg;
	int len, pageoff, i, rc;
	struct rpcrdma_mw *mw;

	mw = seg1->rl_mw;
	seg1->rl_mw = NULL;
	if (!mw) {
		mw = rpcrdma_get_mw(r_xprt);
		if (!mw)
			return -ENOMEM;
	} else {
		/* this is a retransmit; generate a fresh rkey */
		rc = __fmr_unmap(mw);
		if (rc)
			return rc;
	}

	pageoff = offset_in_page(seg1->mr_offset);
	seg1->mr_offset -= pageoff;	/* start of page */
	seg1->mr_len += pageoff;
	len = -pageoff;
	if (nsegs > RPCRDMA_MAX_FMR_SGES)
		nsegs = RPCRDMA_MAX_FMR_SGES;
	for (i = 0; i < nsegs;) {
		rpcrdma_map_one(device, seg, direction);
		mw->fmr.physaddrs[i] = seg->mr_dma;
		len += seg->mr_len;
		++seg;
		++i;
		/* Check for holes */
		if ((i < nsegs && offset_in_page(seg->mr_offset)) ||
		    offset_in_page((seg-1)->mr_offset + (seg-1)->mr_len))
			break;
	}

	rc = ib_map_phys_fmr(mw->fmr.fmr, mw->fmr.physaddrs,
			     i, seg1->mr_dma);
	if (rc)
		goto out_maperr;

	seg1->rl_mw = mw;
	seg1->mr_rkey = mw->fmr.fmr->rkey;
	seg1->mr_base = seg1->mr_dma + pageoff;
	seg1->mr_nsegs = i;
	seg1->mr_len = len;
	return i;

out_maperr:
	dprintk("RPC:       %s: ib_map_phys_fmr %u@0x%llx+%i (%d) status %i\n",
		__func__, len, (unsigned long long)seg1->mr_dma,
		pageoff, i, rc);
	while (i--)
		rpcrdma_unmap_one(device, --seg);
	return rc;
}

static void
__fmr_dma_unmap(struct rpcrdma_xprt *r_xprt, struct rpcrdma_mr_seg *seg)
{
	struct ib_device *device = r_xprt->rx_ia.ri_device;
	int nsegs = seg->mr_nsegs;

	while (nsegs--)
		rpcrdma_unmap_one(device, seg++);
}

/* Invalidate all memory regions that were registered for "req".
 *
 * Sleeps until it is safe for the host CPU to access the
 * previously mapped memory regions.
 */
static void
fmr_op_unmap_sync(struct rpcrdma_xprt *r_xprt, struct rpcrdma_req *req)
{
	struct rpcrdma_mr_seg *seg;
	unsigned int i, nchunks;
	struct rpcrdma_mw *mw;
	LIST_HEAD(unmap_list);
	int rc;

	dprintk("RPC:       %s: req %p\n", __func__, req);

	/* ORDER: Invalidate all of the req's MRs first
	 *
	 * ib_unmap_fmr() is slow, so use a single call instead
	 * of one call per mapped MR.
	 */
	for (i = 0, nchunks = req->rl_nchunks; nchunks; nchunks--) {
		seg = &req->rl_segments[i];
		mw = seg->rl_mw;

		list_add(&mw->fmr.fmr->list, &unmap_list);

		i += seg->mr_nsegs;
	}
	rc = ib_unmap_fmr(&unmap_list);
	if (rc)
		pr_warn("%s: ib_unmap_fmr failed (%i)\n", __func__, rc);

	/* ORDER: Now DMA unmap all of the req's MRs, and return
	 * them to the free MW list.
	 */
	for (i = 0, nchunks = req->rl_nchunks; nchunks; nchunks--) {
		seg = &req->rl_segments[i];

		__fmr_dma_unmap(r_xprt, seg);
		rpcrdma_put_mw(r_xprt, seg->rl_mw);

		i += seg->mr_nsegs;
		seg->mr_nsegs = 0;
		seg->rl_mw = NULL;
	}

	req->rl_nchunks = 0;
}

/* Use a slow, safe mechanism to invalidate all memory regions
 * that were registered for "req".
 *
 * In the asynchronous case, DMA unmapping occurs first here
 * because the rpcrdma_mr_seg is released immediately after this
 * call. It's contents won't be available in __fmr_dma_unmap later.
 * FIXME.
 */
static void
fmr_op_unmap_safe(struct rpcrdma_xprt *r_xprt, struct rpcrdma_req *req,
		  bool sync)
{
	struct rpcrdma_mr_seg *seg;
	struct rpcrdma_mw *mw;
	unsigned int i;

	for (i = 0; req->rl_nchunks; req->rl_nchunks--) {
		seg = &req->rl_segments[i];
		mw = seg->rl_mw;

		if (sync) {
			/* ORDER */
			__fmr_unmap(mw);
			__fmr_dma_unmap(r_xprt, seg);
			rpcrdma_put_mw(r_xprt, mw);
		} else {
			__fmr_dma_unmap(r_xprt, seg);
			__fmr_queue_recovery(mw);
		}

		i += seg->mr_nsegs;
		seg->mr_nsegs = 0;
		seg->rl_mw = NULL;
	}
}

static void
fmr_op_destroy(struct rpcrdma_buffer *buf)
{
	struct rpcrdma_mw *r;
	int rc;

	while (!list_empty(&buf->rb_all)) {
		r = list_entry(buf->rb_all.next, struct rpcrdma_mw, mw_all);
		list_del(&r->mw_all);
		kfree(r->fmr.physaddrs);

		rc = ib_dealloc_fmr(r->fmr.fmr);
		if (rc)
			dprintk("RPC:       %s: ib_dealloc_fmr failed %i\n",
				__func__, rc);

		kfree(r);
	}
}

const struct rpcrdma_memreg_ops rpcrdma_fmr_memreg_ops = {
	.ro_map				= fmr_op_map,
	.ro_unmap_sync			= fmr_op_unmap_sync,
	.ro_unmap_safe			= fmr_op_unmap_safe,
	.ro_open			= fmr_op_open,
	.ro_maxpages			= fmr_op_maxpages,
	.ro_init			= fmr_op_init,
	.ro_destroy			= fmr_op_destroy,
	.ro_displayname			= "fmr",
};
