/*
 * Library Slab Allocator (SLIB)
 *
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "sim.h"
#include "sim-assert.h"
#include <linux/page-flags.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <mm/slab.h>
#include <linux/slib_def.h>

/* glues */
struct list_head slab_caches;
struct kmem_cache *kmem_cache;

static unsigned int calculate_alignment(slab_flags_t flags,
		unsigned int align, unsigned int size)
{
	/*
	 * If the user wants hardware cache aligned objects then follow that
	 * suggestion if the object is sufficiently large.
	 *
	 * The hardware cache alignment cannot override the specified
	 * alignment though. If that is greater then use it.
	 */
	if (flags & SLAB_HWCACHE_ALIGN) {
		unsigned int ralign;

		ralign = cache_line_size();
		while (size <= ralign / 2)
			ralign /= 2;
		align = max(align, ralign);
	}

	if (align < ARCH_SLAB_MINALIGN)
		align = ARCH_SLAB_MINALIGN;

	return ALIGN(align, sizeof(void *));
}


static struct kmem_cache *create_cache(const char *name,
		unsigned int object_size, unsigned int align,
		slab_flags_t flags, unsigned int useroffset,
		unsigned int usersize, void (*ctor)(void *),
		struct kmem_cache *root_cache)
{
	struct kmem_cache *s;
	int err;

	if (WARN_ON(useroffset + usersize > object_size))
		useroffset = usersize = 0;

	err = -ENOMEM;
	s = kmem_cache_zalloc(kmem_cache, GFP_KERNEL);
	if (!s)
		goto out;

	s->name = name;
	s->size = s->object_size = object_size;
	s->align = align;
	s->ctor = ctor;
	s->useroffset = useroffset;
	s->usersize = usersize;

	err = __kmem_cache_create(s, flags);
	if (err)
		goto out_free_cache;

	s->refcount = 1;
	list_add(&s->list, &slab_caches);
out:
	if (err)
		return ERR_PTR(err);
	return s;

out_free_cache:
	kmem_cache_free(kmem_cache, s);
	goto out;
}

struct kmem_cache *files_cachep;

void kfree(const void *p)
{
	unsigned long start;

	if (p == 0)
		return;
	start = (unsigned long)p;
	start -= sizeof(size_t);
	lib_free((void *)start);
}
size_t ksize(const void *p)
{
	size_t *psize = (size_t *)p;

	psize--;
	return *psize;
}
void *__kmalloc(size_t size, gfp_t flags)
{
	void *p = lib_malloc(size + sizeof(size));	
	unsigned long start;

	if (!p){
		return NULL;
	}

	if (p != 0 && (flags & __GFP_ZERO)){
		lib_memset(p, 0, size + sizeof(size));		
	}
	lib_memcpy(p, &size, sizeof(size));	
	start = (unsigned long)p;
	return (void *)(start + sizeof(size));
}

void *__kmalloc_track_caller(size_t size, gfp_t flags, unsigned long caller)
{
	return kmalloc(size, flags);
}

void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
	void *ret;

	if (!new_size) {
		kfree(p);
		return ZERO_SIZE_PTR;
	}

	ret = __kmalloc(new_size, flags);
	if (ret && p != ret)
		kfree(p);

	return ret;
}

struct kmem_cache *
kmem_cache_create(const char *name, unsigned int size,
			unsigned int align, slab_flags_t flags,
			void (*ctor)(void *))
{	
	struct kmem_cache *cache = kmalloc(sizeof(struct kmem_cache), flags);

	if (!cache)
		return NULL;
	cache->name = name;
	cache->size = size;	
	cache->align = align;
	cache->flags = flags;
	cache->ctor = ctor;
	return cache;
}

int __kmem_cache_create(struct kmem_cache * cachep, slab_flags_t flags)
{
	struct kmem_cache *cache = kmalloc(sizeof(struct kmem_cache), flags);
	cache->flags = flags;
	return kmem_cache;
}
void kmem_cache_destroy(struct kmem_cache *cache)
{
	kfree(cache);
}
int kmem_cache_shrink(struct kmem_cache *cache)
{
	return 1;
}
const char *kmem_cache_name(struct kmem_cache *cache)
{
	return cache->name;
}
void *kmem_cache_alloc(struct kmem_cache *cache, gfp_t flags)
{		
	void *p = kmalloc(cache->size, flags);	
	if (p == 0)
		return NULL;
	if (cache->ctor)
		(cache->ctor)(p);

	return p;

}
void kmem_cache_free(struct kmem_cache *cache, void *p)
{
	kfree(p);
}

struct page *
__alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order, int preferred_nid,
							nodemask_t *nodemask)
{
	void *p;
	struct page *page;
	unsigned long pointer;

	/* typically, called from networking code by alloc_page or */
	/* directly with an order = 0. */
	if (order)
		return NULL;
	p = lib_malloc(sizeof(struct page) + (1 << PAGE_SHIFT));
	if (p != 0)
		lib_memset(p, 0, sizeof(struct page) + (1 << PAGE_SHIFT));
	page = (struct page *)p;

	atomic_set(&page->_refcount, 1);
	page->flags = 0;
	pointer = (unsigned long)page;
	pointer += sizeof(struct page);
	page->virtual = (void *)pointer;
	return page;
}
void __free_pages(struct page *page, unsigned int order)
{
	/* typically, called from networking code by __free_page */
	lib_assert(order == 0);
	lib_free(page);
}

void __put_page(struct page *page)
{
	if (atomic_dec_and_test(&page->_refcount))
		lib_free(page);
}
unsigned long get_zeroed_page(gfp_t gfp_mask)
{
	return __get_free_pages(gfp_mask | __GFP_ZERO, 0);
}

void *alloc_pages_exact(size_t size, gfp_t gfp_mask)
{
	return alloc_pages(gfp_mask, get_order(size));
}

unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
	int size = (1 << order) * PAGE_SIZE;
	void *p = kmalloc(size, gfp_mask);

	return (unsigned long)p;
}
void free_pages(unsigned long addr, unsigned int order)
{
	if (addr != 0)
		kfree((void *)addr);
}

void *vmalloc(unsigned long size)
{
	return lib_malloc(size);
}
void *__vmalloc(unsigned long size, gfp_t gfp_mask)
{
	return kmalloc(size, gfp_mask);
}
void vfree(const void *addr)
{
	lib_free((void *)addr);
}
void *vmalloc_node(unsigned long size, int node)
{
	return lib_malloc(size);
}
void vmalloc_sync_all(void)
{
}
void __percpu *__alloc_percpu_gfp(size_t size, size_t align, gfp_t gfp)
{
	return kzalloc(size, GFP_KERNEL);
}
void *__alloc_percpu(size_t size, size_t align)
{
	return kzalloc(size, GFP_KERNEL);
}
void free_percpu(void __percpu *ptr)
{
	kfree(ptr);
}
void *__alloc_bootmem_nopanic(unsigned long size,
			      unsigned long align,
			      unsigned long goal)
{
	return kzalloc(size, GFP_KERNEL);
}

void * __init memblock_alloc_try_nid(
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr,
			int nid)
{
	return kzalloc(size, GFP_KERNEL);
}

struct kmem_cache *
kmem_cache_create_usercopy(const char *name,
		  unsigned int size, unsigned int align,
		  slab_flags_t flags,
		  unsigned int useroffset, unsigned int usersize,
		  void (*ctor)(void *))
{
	struct kmem_cache *cache = kmalloc(sizeof(struct kmem_cache), flags);

	if (!cache)
		return NULL;
	cache->name = name;
	cache->size = size;
	cache->align = align;
	cache->flags = flags;
	cache->ctor = ctor;
	return cache;
}

int kmem_cache_sanity_check(const char *name, unsigned int size)
{
	return 0;
}

