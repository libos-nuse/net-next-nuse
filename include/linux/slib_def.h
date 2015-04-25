#ifndef _LINUX_SLLB_DEF_H
#define _LINUX_SLLB_DEF_H


struct kmem_cache {
	unsigned int object_size;
	const char *name;
	size_t size;
	size_t align;
	unsigned long flags;
	void (*ctor)(void *);
};

void *__kmalloc(size_t size, gfp_t flags);
void *kmem_cache_alloc(struct kmem_cache *, gfp_t);
static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
	return __kmalloc(size, flags);
}

#endif /* _LINUX_SLLB_DEF_H */
