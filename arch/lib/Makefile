ARCH_DIR := arch/lib
SRCDIR=$(dir $(firstword $(MAKEFILE_LIST)))
DCE_TESTDIR=$(srctree)/tools/testing/libos/
KBUILD_KCONFIG := arch/$(ARCH)/Kconfig

CC = gcc
GCCVERSIONGTEQ48 := $(shell expr `gcc -dumpversion` \>= 4.8)
ifeq "$(GCCVERSIONGTEQ48)" "1"
   NO_TREE_LOOP_OPT += -fno-tree-loop-distribute-patterns
endif


-include $(ARCH_DIR)/objs.mk
-include $(srctree)/.config
include $(ARCH_DIR)/processor.mk

# targets
LIBOS_TOOLS=$(ARCH_DIR)/tools
LIBOS_GIT_REPO=git://github.com/libos-nuse/linux-libos-tools
KERNEL_LIB=liblinux-$(KERNELVERSION).so

ALL_OBJS=$(OBJS) $(KERNEL_LIB) $(modules) $(all-obj-for-clean)

# auto generated files
AUTOGENS=$(CRC32TABLE) $(COMPILE_H) $(BOUNDS_H) $(ARCH_DIR)/timeconst.h $(ARCH_DIR)/linker.lds
COMPILE_H=$(srctree)/include/generated/compile.h
BOUNDS_H=$(srctree)/include/generated/bounds.h

# from lib/Makefile
CRC32TABLE = $(ARCH_DIR)/crc32table.h
hostprogs-y	:= $(srctree)/lib/gen_crc32table
clean-files	:= crc32table.h

# sources and objects
LIB_SRC=\
lib.c lib-device.c lib-socket.c random.c softirq.c time.c \
timer.c hrtimer.c sched.c workqueue.c \
print.c slab.c tasklet.c tasklet-hrtimer.c \
glue.c fs.c sysctl.c proc.c sysfs.c \
capability.c pid.c modules.c filemap.c vmscan.c

LIB_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(LIB_SRC))))

DEPENDS=$(addprefix $(ARCH_DIR)/.,\
	$(addsuffix .d,$(basename $(LIB_SRC)))\
	)

# options
COV?=no
cov_yes=-fprofile-arcs -ftest-coverage
cov_no=
covl_yes=-fprofile-arcs
covl_no=
OPT?=yes
opt_yes=-O3 -fomit-frame-pointer $(NO_TREE_LOOP_OPT)
opt_no=-O0
PIC?=yes
pic_yes=-fpic -DPIC
pic_no=-mcmodel=large
PIC_CFLAGS=$(pic_$(PIC))

# flags
CFLAGS_USPACE= \
 $(opt_$(OPT)) -g3 -Wall -Wstrict-prototypes -Wno-trigraphs \
 -fno-inline -fno-strict-aliasing -fno-common \
 -fno-delete-null-pointer-checks -fno-builtin \
 -fno-stack-protector -Wno-unused -Wno-pointer-sign \
 $(PIC_CFLAGS) -D_DEBUG $(cov_$(COV)) -I$(ARCH_DIR)/include

CFLAGS+= \
 $(CFLAGS_USPACE) -nostdinc -D__KERNEL__ -iwithprefix $(srctree)/include \
 -DKBUILD_BASENAME=\"clnt\" -DKBUILD_MODNAME=\"nsc\" -DMODVERSIONS \
 -DEXPORT_SYMTAB \
 -U__FreeBSD__ -D__linux__=1 -Dlinux=1 -D__linux=1 \
 -DCONFIG_DEFAULT_HOSTNAME=\"lib\" \
 -I$(ARCH_DIR)/include/generated/uapi \
 -I$(ARCH_DIR)/include/generated \
 -I$(srctree)/include -I$(ARCH_DIR)/include/uapi \
 -I$(srctree)/include/uapi -I$(srctree)/include/generated/uapi \
 -include $(srctree)/include/linux/kconfig.h \
 -I$(ARCH_DIR) -I.

ifeq ($(PROCESSOR_SIZE),64)
CFLAGS+= -DCONFIG_64BIT
endif

LDFLAGS += -shared -nodefaultlibs -g3 -Wl,-O1 -Wl,-T$(ARCH_DIR)/linker.lds $(covl_$(COV))

# targets

modules:=
all-obj-for-clean:=

all: library modules

# note: the directory order below matters to ensure that we match the kernel order
dirs=kernel/ kernel/time/ kernel/rcu/ kernel/locking/ kernel/bpf/ mm/ fs/ fs/proc/ crypto/ lib/ drivers/base/ drivers/net/ net/ init/
empty:=
space:= $(empty) $(empty)
colon:= :
comma= ,
kernel/_to_keep=notifier.o params.o sysctl.o \
rwsem.o semaphore.o kfifo.o cred.o user.o groups.o ksysfs.o
kernel/time/_to_keep=time.o
kernel/rcu_to_keep=rcu/srcu.o rcu/pdate.o rcu/tiny.o
kernel/locking_to_keep=locking/mutex.o
kernel/bpf_to_keep=bpf/core.o
mm/_to_keep=util.o list_lru.o
crypto/_to_keep=aead.o ahash.o shash.o api.o algapi.o cipher.o compress.o proc.o \
crc32c_generic.o
drivers/base/_to_keep=class.o core.o bus.o dd.o driver.o devres.o module.o map.o
drivers/net/_to_keep=loopback.o
lib/_to_keep=klist.o kobject.o kref.o hweight.o int_sqrt.o checksum.o \
find_last_bit.o find_next_bit.o bitmap.o nlattr.o idr.o libcrc32c.o \
ctype.o string.o kasprintf.o rbtree.o sha1.o textsearch.o vsprintf.o \
rwsem-spinlock.o scatterlist.o ratelimit.o hexdump.o dec_and_lock.o \
div64.o dynamic_queue_limits.o md5.o kstrtox.o iovec.o lockref.o crc32.o \
rhashtable.o iov_iter.o cmdline.o kobject_uevent.o
fs/_to_keep=read_write.o libfs.o namei.o filesystems.o file.o file_table.o \
dcache.o inode.o pipe.o char_dev.o splice.o no-block.o seq_file.o super.o \
fcntl.o coredump.o
fs/proc/_to_keep=proc_sysctl.o proc_net.o root.o generic.o inode.o
init/_to_keep=version.o

quiet_cmd_objsmk = OBJS-MK   $@
      cmd_objsmk = \
	for i in 1; do \
	$(foreach d,$(dirs), \
           $(MAKE) -i -s -f $< srcdir=$(srctree)/$(d) \
	    objdir=$(srctree)/$(d) \
            config=$(srctree)/.config \
	    to_keep=$(subst $(space),$(colon),$($(d)_to_keep)) print;) \
	done > $@

$(ARCH_DIR)/objs.mk: $(ARCH_DIR)/Makefile.print $(srctree)/.config $(ARCH_DIR)/Makefile
	$(call if_changed,objsmk)

quiet_cmd_timeconst = GEN     $@
      cmd_timeconst = echo "hz=$(CONFIG_HZ)" > $(ARCH_DIR)/hz.bc ; \
                      bc $(ARCH_DIR)/hz.bc kernel/time/timeconst.bc > $@
$(ARCH_DIR)/timeconst.h: $(srctree)/.config
	$(call if_changed,timeconst)

quiet_cmd_linker = GEN     $@
      cmd_linker = ld -shared --verbose | ./$^ > $@
$(ARCH_DIR)/linker.lds: $(ARCH_DIR)/generate-linker-script.py
	$(call if_changed,linker)

quiet_cmd_crc32src = GEN     $@
      cmd_crc32src = $(MAKE) -f $(srctree)/Makefile silentoldconfig ; \
                     cc $^ -o $@
$(srctree)/lib/gen_crc32table: $(srctree)/lib/gen_crc32table.c
	$(call if_changed,crc32src)

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(CRC32TABLE): $(srctree)/lib/gen_crc32table
	$(call if_changed,crc32)

# copied from init/Makefile
       chk_compile.h = :
 quiet_chk_compile.h = echo '  CHK     $@'
silent_chk_compile.h = :
$(COMPILE_H): include/generated/utsrelease.h asm-generic $(version_h)
	@$($(quiet)chk_compile.h)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkcompile_h $@ \
	"$(UTS_MACHINE)" "$(CONFIG_SMP)" "$(CONFIG_PREEMPT)" "$(CC) $(KBUILD_CFLAGS)"

# crafted from $(srctree)/Kbuild
quiet_cmd_lib_bounds = GEN     $@
define cmd_lib_bounds
	(set -e; \
	 echo "#ifndef GENERATED_BOUNDS_H"; \
	 echo "#define GENERATED_BOUNDS_H"; \
	 echo ""; \
	 echo "#define NR_PAGEFLAGS (__NR_PAGEFLAGS)"; \
	 echo "#define MAX_NR_ZONES (__MAX_NR_ZONES)"; \
	 echo ""; \
	 echo "#endif /* GENERATED_BOUNDS_H */") > $@
endef

$(BOUNDS_H): FORCE
	$(Q)mkdir -p $(dir $@)
	$(call cmd,lib_bounds)


KERNEL_BUILTIN=$(addprefix $(srctree)/,$(addsuffix builtin.o,$(dirs)))
OBJS=$(LIB_OBJ) $(foreach builtin,$(KERNEL_BUILTIN),$(if $($(builtin)),$($(builtin))))
export OBJS KERNEL_LIB COV covl_yes covl_no

quiet_cmd_cc = CC   $@
      cmd_cc = 	mkdir -p $(dir $@);	\
		$(CC) $(CFLAGS) -c $< -o $@
quiet_cmd_linkko = KO   $@
      cmd_linkko = $(CC) -shared -o $@ -nostdlib $^
quiet_cmd_builtin = BUILTIN   $@
      cmd_builtin = mkdir -p $(dir $(srctree)/$@); rm -f $(srctree)/$@; \
		    if test -n "$($(srctree)/$@)"; then for f in $($(srctree)/$@); \
		    do $(AR) Tcru $@ $$f; done; else $(AR) Tcru $@; fi

%/builtin.o:
	$(call if_changed,builtin)
%.ko:%.o
	$(call if_changed,linkko)
%.o:%.c
	$(call if_changed,cc)

library: $(KERNEL_LIB) $(LIBOS_TOOLS)
modules: $(modules)

$(LIBOS_TOOLS): $(KERNEL_LIB) Makefile FORCE
	$(Q) if [ ! -d "$@" ]; then \
		git clone $(LIBOS_GIT_REPO) $@ ;\
	fi
	$(Q) $(MAKE) -C $(LIBOS_TOOLS)

install: modules library

install-dir:

$(KERNEL_LIB): $(ARCH_DIR)/objs.mk $(AUTOGENS) $(OBJS)
	$(call if_changed,linklib)

quiet_cmd_linklib = LIB		$@
      cmd_linklib = $(CC) -Wl,--whole-archive $(OBJS) $(LDFLAGS) -o $@; \
		    ln -s -f $(KERNEL_LIB) liblinux.so

quiet_cmd_clean = CLEAN   $@
      cmd_clean = for f in $(foreach m,$(modules),$($(m))) ; do rm -f $$f 2>/dev/null; done ; \
		  for f in $(ALL_OBJS); do rm -f $$f; done 2>/dev/null ;\
		  rm -rf $(AUTOGENS) $(ARCH_DIR)/objs.mk 2>/dev/null ;\
		  if [ -d $(LIBOS_TOOLS) ]; then $(MAKE) -C $(LIBOS_TOOLS) clean ; fi

archclean:
	$(call if_changed,clean)

.%.d:%.c $(srctree)/.config
	$(Q) set -e; $(CC) -MM -MT $(<:.c=.o) $(CFLAGS) $< > $@

deplib: $(DEPENDS)
	-include $(DEPENDS)

test:
	$(Q) $(MAKE) -C $(DCE_TESTDIR)/

.PHONY : clean deplib

