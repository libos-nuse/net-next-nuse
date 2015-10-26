#ifndef _ASM_SIM_IO_H
#define _ASM_SIM_IO_H

/* Basic port I/O */
static inline void outb(u8 v, u16 port)
{
	asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}
static inline u8 inb(u16 port)
{
	u8 v;
	asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline void outw(u16 v, u16 port)
{
	asm volatile("outw %0,%1" : : "a" (v), "dN" (port));
}
static inline u16 inw(u16 port)
{
	u16 v;
	asm volatile("inw %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

static inline void outl(u32 v, u16 port)
{
	asm volatile("outl %0,%1" : : "a" (v), "dN" (port));
}
static inline u32 inl(u16 port)
{
	u32 v;
	asm volatile("inl %1,%0" : "=a" (v) : "dN" (port));
	return v;
}

#ifndef outb
#define outb outb
#endif
#ifndef outw
#define outw outw
#endif
#ifndef outl
#define outl outl
#endif
#ifndef inb
#define inb inb
#endif
#ifndef inw
#define inw inw
#endif
#ifndef inl
#define inl inl
#endif


#include <asm-generic/io.h>

#define ioremap(physaddr, size)	((void __iomem *)(unsigned long)(physaddr))
#define iounmap(addr)		((void)0)
#define ioremap_wc ioremap_nocache

#ifndef ioremap_nocache
#define ioremap_nocache ioremap_nocache
static inline void __iomem *ioremap_nocache(phys_addr_t offset, size_t size)
{
	return ioremap(offset, size);
}
#endif

#endif /* _ASM_SIM_IO_H */
