/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.sun.com/software/products/lustre/docs/GPLv2.pdf
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, 2012, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 */

#ifndef _LL_H
#define _LL_H

/** \defgroup lite lite
 *
 * @{
 */

#include "linux/lustre_lite.h"

#include "obd_class.h"
#include "lustre_net.h"
#include "lustre_mds.h"
#include "lustre_ha.h"

/* 4UL * 1024 * 1024 */
#define LL_MAX_BLKSIZE_BITS     (22)
#define LL_MAX_BLKSIZE	  (1UL<<LL_MAX_BLKSIZE_BITS)

/*
 * This is embedded into llite super-blocks to keep track of
 * connect flags (capabilities) supported by all imports given mount is
 * connected to.
 */
struct lustre_client_ocd {
	/*
	 * This is conjunction of connect_flags across all imports (LOVs) this
	 * mount is connected to. This field is updated by cl_ocd_update()
	 * under ->lco_lock.
	 */
	__u64	      lco_flags;
	struct mutex	   lco_lock;
	struct obd_export *lco_md_exp;
	struct obd_export *lco_dt_exp;
};

/*
 * Chain of hash overflow pages.
 */
struct ll_dir_chain {
	/* XXX something. Later */
};

static inline void ll_dir_chain_init(struct ll_dir_chain *chain)
{
}

static inline void ll_dir_chain_fini(struct ll_dir_chain *chain)
{
}

static inline unsigned long hash_x_index(__u64 hash, int hash64)
{
	if (BITS_PER_LONG == 32 && hash64)
		hash >>= 32;
	/* save hash 0 as index 0 because otherwise we'll save it at
	 * page index end (~0UL) and it causes truncate_inode_pages_range()
	 * to loop forever.
	 */
	return ~0UL - (hash + !hash);
}

/** @} lite */

#endif
