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
#include <linux/slib_def.h>

/* glues */
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

	if (!p)
		return NULL;

	if (p != 0 && (flags & __GFP_ZERO))
		lib_memset(p, 0, size + sizeof(size));
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
kmem_cache_create(const char *name, size_t size, size_t align,
		  unsigned long flags, void (*ctor)(void *))
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

void put_page(struct page *page)
{
	if (atomic_dec_and_test(&page->_count))
		lib_free(page);
}

void *vmalloc(unsigned long size)
{
	return lib_malloc(size);
}
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
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
