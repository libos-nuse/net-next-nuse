#include "sim.h"
#include "sim-assert.h"
#include <linux/page-flags.h>
#include <linux/types.h>
#include <linux/slab.h>

void kfree(const void *p)
{
  if (p == 0)
    {
      return;
    }
  unsigned long start = (unsigned long) p;
  start -= sizeof (size_t);
  sim_free ((void*)start);
}
size_t ksize(const void *p)
{
  size_t *psize = (size_t *)p;
  psize--;
  return *psize;
}
void *__kmalloc(size_t size, gfp_t flags)
{
  void *p = sim_malloc (size + sizeof (size));
  if (!p)
    {
      return NULL;
    }

  if (p != 0 && (flags & __GFP_ZERO))
    {
      sim_memset (p, 0, size + sizeof(size));
    }
  sim_memcpy (p, &size, sizeof(size));
  unsigned long start = (unsigned long) p;
  return (void*)(start + sizeof (size));
}

void *kmalloc(size_t size, gfp_t flags)
{
  return __kmalloc (size, flags);
}
void *__kmalloc_track_caller(size_t size, gfp_t flags, unsigned long caller)
{
  return kmalloc (size, flags);
}

struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t align,
		  unsigned long flags, void (*ctor)(void *))
{
  struct kmem_cache *cache = kmalloc (sizeof (struct kmem_cache), flags);
  if (!cache)
    {
      return NULL;
    }
  cache->name = name;
  cache->size = size;
  cache->align = align;
  cache->flags = flags;
  cache->ctor = ctor;
  return cache;
}
void kmem_cache_destroy(struct kmem_cache *cache)
{
  kfree (cache);
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
    {
      return NULL;
    }
  if (cache->ctor)
    {
      (cache->ctor) (p);
    }
  return p;

}
void kmem_cache_free(struct kmem_cache *cache, void *p)
{
  kfree (p);
}

struct page *
__alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order,
                       struct zonelist *zonelist, nodemask_t *nodemask)
{
  // typically, called from networking code by alloc_page or
  // directly with an order = 0.
  if (order)
    {
      return NULL;
    }
  void *p = sim_malloc (sizeof (struct page) + (1<<PAGE_SHIFT));
  struct page *page = (struct page *)p;
  atomic_set (&page->_count, 1);
  page->flags = 0;
  unsigned long pointer = (unsigned long) page;
  pointer += sizeof (struct page);
  page->virtual = (void*)pointer;
  return page;
}
void __free_pages(struct page *page, unsigned int order)
{
  // typically, called from networking code by __free_page
  sim_assert (order == 0);
  sim_free (page);
}

void put_page(struct page *page)
{
  if (atomic_dec_and_test (&page->_count))
    {
      sim_free (page);
    }
}
unsigned long get_zeroed_page(gfp_t gfp_mask)
{
  return __get_free_pages(gfp_mask | __GFP_ZERO, 0);
}

void *alloc_pages_exact(size_t size, gfp_t gfp_mask)
{
  return alloc_pages (gfp_mask, get_order (size));
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
    {
      kfree ((void*)addr);
    }
}

void *vmalloc(unsigned long size)
{
  return sim_malloc (size);
}
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
{
  return kmalloc (size, gfp_mask);
}
void vfree(const void *addr)
{
  sim_free ((void*)addr);
}
void *vmalloc_node(unsigned long size, int node)
{
  return sim_malloc (size);
}
void vmalloc_sync_all(void)
{}
void *__alloc_percpu(size_t size, size_t align)
{
  return kzalloc (size, GFP_KERNEL);
}
void free_percpu(void __percpu *ptr)
{
  kfree (ptr);
}
void *__alloc_bootmem_nopanic(unsigned long size,
			      unsigned long align,
			      unsigned long goal)
{
  return kzalloc (size, GFP_KERNEL);
}
