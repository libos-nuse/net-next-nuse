#include <asm-generic/io.h>

#define ioremap(physaddr, size)	((void __iomem *)(unsigned long)(physaddr))
#define iounmap(addr)		((void)0)
