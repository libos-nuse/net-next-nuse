ARCH_DIR := arch/lib
KBUILD_DEFCONFIG := defconfig
KBUILD_KCONFIG := $(ARCH_DIR)/Kconfig

DCE_TESTDIR=$(srctree)/tools/testing/libos/
LIBOS_TOOLS=$(ARCH_DIR)/tools
LIBOS_GIT_REPO=git://github.com/libos-nuse/linux-libos-tools
KERNEL_LIB=liblinux-$(KERNELVERSION).so

CC = gcc
GCCVERSIONGTEQ48 := $(shell expr `gcc -dumpversion` \>= 4.8)
ifeq "$(GCCVERSIONGTEQ48)" "1"
   NO_TREE_LOOP_OPT += -fno-tree-loop-distribute-patterns
endif

include $(ARCH_DIR)/processor.mk

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

KBUILD_CFLAGS := \
 $(CFLAGS_USPACE) -nostdinc -D__KERNEL__ -iwithprefix $(srctree)/include \
 -DMODVERSIONS -DEXPORT_SYMTAB \
 -U__FreeBSD__ -D__linux__=1 -Dlinux=1 -D__linux=1 \
 -DCONFIG_DEFAULT_HOSTNAME=\"lib\" \
 -I$(ARCH_DIR)/include/generated/uapi \
 -I$(ARCH_DIR)/include/generated \
 -I$(srctree)/include -I$(ARCH_DIR)/include/uapi \
 -I$(srctree)/include/uapi -I$(srctree)/include/generated/uapi \
 -include $(srctree)/include/linux/kconfig.h \
 -I$(ARCH_DIR) -I.

ifeq ($(PROCESSOR_SIZE),64)
KBUILD_CFLAGS+= -DCONFIG_64BIT
endif

PHONY += library

drv-base-objs := $(addprefix drivers/base/, class.o core.o bus.o dd.o driver.o \
devres.o module.o map.o)
drv-net-objs := $(addprefix drivers/net/, loopback.o)
kernel-objs := $(addprefix kernel/, notifier.o params.o sysctl.o user.o \
groups.o ksysfs.o)
kernel-time-objs := $(addprefix kernel/time/, time.o)
kernel-rcu-objs := $(addprefix kernel/rcu/, srcu.o update.o tiny.o)
kernel-locking-objs := $(addprefix kernel/locking/, mutex.o rwsem-spinlock.o rwsem.o)
kernel-bpf-objs := $(addprefix kernel/bpf/, core.o)
mm-objs := $(addprefix mm/, util.o list_lru.o)
crypto-objs := $(addprefix crypto/, aead.o ahash.o shash.o api.o algapi.o \
cipher.o compress.o proc.o crc32c_generic.o)
lib-objs := $(addprefix lib/, klist.o kobject.o hweight.o int_sqrt.o \
checksum.o find_last_bit.o find_next_bit.o bitmap.o nlattr.o idr.o \
libcrc32c.o ctype.o string.o kasprintf.o rbtree.o sha1.o textsearch.o \
vsprintf.o scatterlist.o ratelimit.o hexdump.o dec_and_lock.o \
div64.o dynamic_queue_limits.o md5.o kstrtox.o lockref.o crc32.o \
rhashtable.o iov_iter.o cmdline.o kobject_uevent.o)
fs-objs := $(addprefix fs/, read_write.o libfs.o namei.o filesystems.o \
file.o file_table.o dcache.o inode.o pipe.o char_dev.o splice.o no-block.o \
seq_file.o super.o fcntl.o)# coredump.o)
fs-proc-objs := $(addprefix fs/proc/, proc_sysctl.o proc_net.o root.o \
generic.o inode.o)
init-objs := $(addprefix init/, version.o)
LIB_KOBJS := $(drv-base-objs) $(drv-net-objs) $(kernel-objs) $(kernel-rcu-objs) $(kernel-bpf-objs) \
$(kernel-locking-objs) $(kernel-time-objs) $(mm-objs) $(crypto-objs) $(lib-objs) $(fs-objs) \
$(fs-proc-objs) $(init-objs) net/built-in.o $(ARCH_DIR)/kernel/built-in.o

LDFLAGS_lib += -shared -nodefaultlibs -g3 -Wl,-O1 -Wl,-T$(ARCH_DIR)/linker.lds $(covl_$(COV))
OBJS=$(LIB_KOBJS)

export OBJS KERNEL_LIB COV covl_yes covl_no


$(drv-base-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=drivers/base obj-y="$(drv-base-objs)"
$(drv-net-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=drivers/net obj-y="$(drv-net-objs)" subdir-y=
$(kernel-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=kernel obj-y="$(kernel-objs)"
$(kernel-time-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=kernel/time obj-y="$(kernel-time-objs)"
$(kernel-locking-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=kernel/locking obj-y="$(kernel-locking-objs)"
$(kernel-rcu-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=kernel/rcu obj-y="$(kernel-rcu-objs)"
$(kernel-bpf-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=kernel/bpf obj-y="$(kernel-bpf-objs)"
$(mm-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=mm obj-y="$(mm-objs)"
$(crypto-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=crypto obj-y="$(crypto-objs)"
$(lib-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=lib obj-y="$(lib-objs)" subdir-y=
$(fs-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=fs obj-y="$(fs-objs)"
$(fs-proc-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=fs/proc obj-y="$(fs-proc-objs)"
$(init-objs):
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=init obj-y="$(init-objs)"
net/built-in.o:
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=net
$(ARCH_DIR)/kernel/built-in.o:
	$(Q)$(MAKE) -f ./scripts/Makefile.build obj=$(ARCH_DIR)/kernel

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

include/generated/bounds.h: FORCE
	$(Q)mkdir -p $(dir $@)
	$(call cmd,lib_bounds)

all: library
vmlinux:

module: library

library: archprepare $(KERNEL_LIB) $(LIBOS_TOOLS)

$(LIBOS_TOOLS): $(KERNEL_LIB) Makefile FORCE
	$(Q) if [ ! -d "$@" ]; then \
		git clone $(LIBOS_GIT_REPO) $@ ;\
	fi
	$(Q) $(MAKE) -C $(LIBOS_TOOLS)

quiet_cmd_linker = GEN     $@
      cmd_linker = ld -shared --verbose | $^ > $@
$(ARCH_DIR)/linker.lds: $(ARCH_DIR)/generate-linker-script.py
	$(call if_changed,linker)

quiet_cmd_linklib = LIB     $@
      cmd_linklib = $(CC) -Wl,--whole-archive $(OBJS) $(LDFLAGS_lib) -o $@; \
		    ln -s -f $(KERNEL_LIB) liblinux.so
$(KERNEL_LIB): $(LIB_KOBJS) $(ARCH_DIR)/linker.lds
	$(call if_changed,linklib)

define archhelp
  echo '* library	- Shared library file (liblinux-$(KERNELVERSION).so)'
endef

archprepare: include/generated/utsrelease.h asm-generic $(version_h) \
include/generated/bounds.h

head-y = 
init-y = 
core-y :=
drivers-y :=
libs-y :=
net-y :=

clean-files := $(ARCH_DIR)/linker.lds

test:
	$(Q) $(MAKE) -C $(DCE_TESTDIR)/
