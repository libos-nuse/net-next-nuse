
srctree=../../../../
# CFLAGS are carefully listed because of conflicts between /usr/include
# and (linux-tree)/include/linux for example.
CFLAGS=-g3 -O2 -nostdinc -fno-builtin -D__KERNEL__
CFLAGS+=-I$(srctree)/arch/lib/tools -I$(srctree)/arch/lib/include
CFLAGS+=-I$(srctree)/arch/lib/include/asm/ -I$(srctree)/arch/lib/include/uapi
CFLAGS+=-I$(srctree)/arch/lib/include/generated/ -I$(srctree)/arch/lib/include/generated/uapi
CFLAGS+=-I$(srctree)/include -I$(srctree)/include/uapi
CFLAGS+=$(shell LANG=C $(CC) -print-search-dirs | sed -ne 's/install: \(.*\)/-I \1include/gp')

LDFLAGS=-L$(srctree) -L$(srctree)/arch/lib/tools/ -Wl,-rpath=$(srctree) \
	-Wl,-rpath=$(srctree)/arch/lib/tools -llinux -lnuse-linux
# This messy linker flag should be deleted but current liblinux.so includes 
# so many unresolved symbols which is not used if you're lucky.
LDFLAGS+=-Wl,--unresolved-symbols=ignore-in-shared-libs

NM=nm

#
# objcopy trick is taken from rumpkernel
#
# it is tricky: since all the symbols in linux source tree are _prefixed_ with
# "rumpns_" in order to avoid duplicated symbols between linux kernel code and
# other userspace code, libos creates a namespace with the prefix.
#
# see more information at the commit message:
# https://github.com/libos-nuse/net-next-nuse/commit/751adb6cd217bca14d4ec93d2a686c771ab89ffd
# or the rump kernel book (http://book.rumpkernel.org/) "Chapter 3.2.1 C Symbol Namespaces"
#
# This part of code should be shared with arch/lib/Makefile etc at somewhere else.
#

GCCSYMBOLS=__umoddi3|__udivdi3
EXP_SYMRENAME=main|rump|RUMP|lib_|g_imported|g_kernel|__initcall_start|__initcall_end|__gcov|_end|_GLOBAL_OFFSET_TABLE|${GCCSYMBOLS}'${_SYMQUIRK}'${RUMP_SYM_NORENAME:D|${RUMP_SYM_NORENAME}}
RENAME_SYMBOLS= ${NM} -go $@ | awk ' \
	$$NF!~/^'${_PQ}'(${EXP_SYMRENAME})/ \
	{s=$$NF;sub(/^'${_PQ}'/, "&rumpns_", s); print $$NF, s}'\
	| sort | uniq  > $@.renametab; \
	objcopy --preserve-dates --redefine-syms $@.renametab $@; \
	rm -f $@.renametab

# build target
tcp-terminate: tcp-terminate.o Makefile
	$(CC) $(LDFLAGS) $< -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) $< -c
	$(RENAME_SYMBOLS)
