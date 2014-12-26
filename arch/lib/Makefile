ARCH_DIR := arch/lib
SRCDIR=$(dir $(firstword $(MAKEFILE_LIST)))

CC = gcc
GCCVERSIONGTEQ48 := $(shell expr `gcc -dumpversion` \>= 4.8)
ifeq "$(GCCVERSIONGTEQ48)" "1"
   NO_TREE_LOOP_OPT += -fno-tree-loop-distribute-patterns
endif


NETMAP?=no
netmap_yes=nuse-vif-netmap.c
netmap_no=

DPDK?=no
dpdk_yes=nuse-vif-dpdk.c
dpdk_no=
dpdkl_yes=-L $(RTE_SDK)/$(RTE_TARGET) -L$(RTE_SDK)/$(RTE_TARGET)/lib $(ARCH_DIR)/nuse-vif-dpdk.o $(DPDK_LDLIBS)
dpdkl_no=

DPDK_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(dpdk_$(DPDK)))))
RTE_SDK=$(shell expr `pwd`)/dpdk
RTE_TARGET=build

DPDK_CFLAGS += -include $(RTE_SDK)/$(RTE_TARGET)/include/rte_config.h \
	-I$(RTE_SDK)/$(RTE_TARGET)/include -I$(ARCH_DIR)/include \
	-DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 \
	-DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 \
	-DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX \
	-DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C \
	-DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX,RTE_CPUFLAG_RDRAND,RTE_CPUFLAG_FSGSBASE,RTE_CPUFLAG_F16C

DPDK_LDLIBS += -lrte_pmd_virtio_uio -lrte_pmd_vmxnet3_uio -lrte_pmd_ixgbe \
          -lrte_pmd_e1000 -lrte_timer -lrte_hash \
          -lrte_mbuf -lethdev \
          -lrte_eal -lrte_mempool -lrte_ring -lrte_pmd_ring -lrte_malloc

quiet_cmd_vif_dpdk = VIF-DPDK $@
      cmd_vif_dpdk = \
		    $(CC) $(CFLAGS_USPACE) -m64 -pthread -march=native $(DPDK_CFLAGS) -c $^ -o $@

$(ARCH_DIR)/nuse-vif-dpdk.o: $(ARCH_DIR)/nuse-vif-dpdk.c
ifeq ($(DPDK), yes)
	$(call if_changed,vif_dpdk)
else
	@echo -n ""
endif

dpdk-sdk:
	git submodule init; \
	git submodule update; \
	cd $(srctree)/dpdk ; make CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=y T=$(shell expr `uname -m`)-native-linuxapp-gcc config && \
           make SRCARCH=x86 CONFIG_RTE_BUILD_COMBINE_LIBS=y EXTRA_CFLAGS="-fPIC -g" ;\



NUSE_USPACE_SRC=\
nuse-fiber.c nuse-vif.c nuse-hostcalls.c nuse-config.c \
nuse-vif-rawsock.c nuse-vif-tap.c $(netmap_$(NETMAP))

NUSE_SRC=$(NUSE_USPACE_SRC)\
nuse.c nuse-poll.c nuse-glue.c

SIM_SRC=\
sim.c

LIB_SRC=\
lib.c lib-device.c lib-socket.c random.c softirq.c time.c \
glue.c fs.c sysctl.c timer.c \
hrtimer.c sched.c sysfs.c workqueue.c \
print.c security.c slab.c tasklet.c \
proc.c seq.c socket.c tasklet-hrtimer.c \
cred.c pid.c modules.c filemap.c splice.c vmscan.c \
dcache.c super.c inode.c

DEPENDS=$(addprefix $(ARCH_DIR)/.,\
	$(addsuffix .d,$(basename $(NUSE_SRC)))\
	$(addsuffix .d,$(basename $(SIM_SRC)))\
	$(addsuffix .d,$(basename $(LIB_SRC)))\
	)
-include $(DEPENDS)


COV?=no
cov_yes=-fprofile-arcs -ftest-coverage
cov_no=
covl_yes=-fprofile-arcs
covl_no=
# no idea why OPT=no doesn't work...
OPT?=yes
opt_yes=-O3 -fomit-frame-pointer $(NO_TREE_LOOP_OPT)
opt_no=-O0
PIC?=yes
pic_yes=-fpic -DPIC
pic_no=-mcmodel=large
PIC_CFLAGS=$(pic_$(PIC))
NUSE_LIB=libnuse-linux$(KERNELVERSION).so
SIM_LIB=libsim-linux$(KERNELVERSION).so
KERNEL_LIB=$(NUSE_LIB) $(SIM_LIB)
SIM_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(SIM_SRC))))
NUSE_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(NUSE_SRC))))
LIB_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(LIB_SRC))))

CFLAGS_USPACE= \
 $(opt_$(OPT)) -g3 -Wall -Wstrict-prototypes -Wno-trigraphs \
 -fno-inline -fno-strict-aliasing -fno-common \
 -fno-delete-null-pointer-checks \
 -fno-stack-protector -Wno-unused -Wno-pointer-sign \
 $(PIC_CFLAGS) -D_DEBUG $(cov_$(COV))

CFLAGS+= \
 $(CFLAGS_USPACE) -D__KERNEL__ -iwithprefix $(srctree)/include \
 -DKBUILD_BASENAME=\"clnt\" -DKBUILD_MODNAME=\"nsc\" -DMODVERSIONS \
 -DEXPORT_SYMTAB -include $(srctree)/include/linux/kconfig.h \
 -U__FreeBSD__ -D__linux__=1 -Dlinux=1 -D__linux=1 \
 -I$(ARCH_DIR)/include -I$(srctree)/include \
 -I$(ARCH_DIR) -I$(srctree) -I$(srctree) -I$(srctree)/include/uapi \
 -DCONFIG_DEFAULT_HOSTNAME=\"lib\"

include $(ARCH_DIR)/processor.mk

ifeq ($(PROCESSOR_SIZE),64)
CFLAGS+= -DCONFIG_64BIT
endif

LDFLAGS += -shared -nodefaultlibs -g3 -Wl,-O1 -Wl,-T$(ARCH_DIR)/linker.lds $(covl_$(COV))
LDFLAGS_NUSE = -ldl -lpthread -lrt
LDFLAGS_SIM =

modules:=
all-obj-for-clean:=

all: library modules

-include $(ARCH_DIR)/objs.mk

AUTOCONF=$(ARCH_DIR)/autoconf.h
COMPILE_H=$(srctree)/include/generated/compile.h
# note: the directory order below matters to ensure that we match the kernel order
dirs=kernel/ kernel/time/ kernel/rcu/ kernel/locking/ kernel/bpf/ mm/ fs/ fs/proc/ crypto/ lib/ drivers/base/ drivers/net/ net/ init/
empty:=
space:= $(empty) $(empty)
colon:= :
comma= ,
kernel/_to_keep=notifier.o params.o sysctl.o \
rwsem.o semaphore.o kfifo.o
kernel/time/_to_keep=time.o
kernel/rcu_to_keep=rcu/srcu.o rcu/pdate.o rcu/tiny.o
kernel/locking_to_keep=locking/mutex.o
kernel/bpf_to_keep=bpf/core.o
mm/_to_keep=util.o list_lru.o iov_iter.o
crypto/_to_keep=aead.o ahash.o shash.o api.o algapi.o cipher.o compress.o proc.o \
crc32c_generic.o
drivers/base/_to_keep=class.o core.o bus.o dd.o driver.o devres.o module.o
drivers/net/_to_keep=loopback.o
lib/_to_keep=klist.o kobject.o kref.o hweight.o int_sqrt.o checksum.o \
find_last_bit.o find_next_bit.o bitmap.o nlattr.o idr.o libcrc32c.o \
ctype.o string.o kasprintf.o rbtree.o sha1.o textsearch.o vsprintf.o \
rwsem-spinlock.o scatterlist.o ratelimit.o hexdump.o dec_and_lock.o \
div64.o dynamic_queue_limits.o md5.o kstrtox.o iovec.o lockref.o crc32.o \
rhashtable.o
fs/_to_keep=read_write.o libfs.o namei.o
fs/proc/_to_keep=proc_sysctl.o
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

-include $(srctree)/.config

quiet_cmd_autoconf = GEN     $@
      cmd_autoconf = $(ARCH_DIR)/generate-autoconf.py $(srctree)/.config > $@
$(AUTOCONF): $(ARCH_DIR)/generate-autoconf.py $(srctree)/.config $(ARCH_DIR)/timeconst.h
	$(call if_changed,autoconf)
quiet_cmd_timeconst = GEN     $@
      cmd_timeconst = echo "hz=$(CONFIG_HZ)" > $(ARCH_DIR)/hz.bc ; \
                      bc $(ARCH_DIR)/hz.bc kernel/time/timeconst.bc > $@
$(ARCH_DIR)/timeconst.h: $(srctree)/.config
	$(call if_changed,timeconst)
quiet_cmd_linker = GEN     $@
      cmd_linker = ld -shared --verbose | ./$^ > $@
$(ARCH_DIR)/linker.lds: $(ARCH_DIR)/generate-linker-script.py
	$(call if_changed,linker)

# from lib/Makefile
CRC32TABLE = $(ARCH_DIR)/crc32table.h
hostprogs-y	:= $(srctree)/lib/gen_crc32table
clean-files	:= crc32table.h

quiet_cmd_crc32src = GEN     $@
      cmd_crc32src = cc $^ -o $@
$(srctree)/lib/gen_crc32table: $(srctree)/lib/gen_crc32table.c
	$(call if_changed,crc32src)

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(CRC32TABLE): $(srctree)/lib/gen_crc32table
	$(call if_changed,crc32)

# copy from init/Makefile
       chk_compile.h = :
 quiet_chk_compile.h = echo '  CHK     $@'
silent_chk_compile.h = :
$(COMPILE_H): include/generated/utsrelease.h
	@$($(quiet)chk_compile.h)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkcompile_h $@ \
	"$(UTS_MACHINE)" "$(CONFIG_SMP)" "$(CONFIG_PREEMPT)" "$(CC) $(KBUILD_CFLAGS)"

KERNEL_BUILTIN=$(addprefix $(srctree)/,$(addsuffix builtin.o,$(dirs)))
OBJS=$(COMPILE_H) $(LIB_OBJ) $(foreach builtin,$(KERNEL_BUILTIN),$(if $($(builtin)),$($(builtin))))
quiet_cmd_cc = CC   $@
      cmd_cc = 	mkdir -p $(dir $@);	\
		$(CC) $(CFLAGS) -c $< -o $@
quiet_cmd_linkko = KO   $@
      cmd_linkko = $(CC) -shared -o $@ -nostdlib $^
quiet_cmd_builtin = BUILTIN   $@
      cmd_builtin = mkdir -p $(dir $(srctree)/$@); if test -n "$($(srctree)/$@)"; then for f in $($(srctree)/$@); \
		    do $(AR) Tcru $@ $$f; done; else $(AR) Tcru $@; fi

# XXX: no idea how to handle these exception cleanly..
quiet_cmd_ccusp = CC   $@
      cmd_ccusp = mkdir -p $(dir $@);	\
		$(CC) $(CFLAGS_USPACE) -c $< -o $@

NUSE_USPACE_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(NUSE_USPACE_SRC))))
$(NUSE_USPACE_OBJ): %.o : %.c
	$(call if_changed,ccusp)

%/builtin.o:
	$(call if_changed,builtin)
%.ko:%.o
	$(call if_changed,linkko)
%.o:%.c
	$(call if_changed,cc)

library: $(KERNEL_LIB)
modules: $(modules)

quiet_cmd_nusetest = BUILDTEST   $@
      cmd_nusetest = $(MAKE) -s -C $(ARCH_DIR)/test/nuse

nuse: $(ARCH_DIR)/test/nuse
	$(call if_changed,nusetest)

install: modules library

install-dir:

quiet_cmd_clean = CLEAN   $@
      cmd_clean = for f in $(foreach m,$(modules),$($(m))) ; do rm -f $$f 2>/dev/null; done ; \
		  for f in $(ALL_OBJS) $(KERNEL_LIB) $(modules) $(all-obj-for-clean); do rm -f $$f; done 2>/dev/null ;\
		  rm -rf $(ARCH_DIR)/linker.lds $(AUTOCONF) $(ARCH_DIR)/objs.mk $(ARCH_DIR)/timeconst.h 2>/dev/null ;\
		  rm -f $(DEPENDS) ;\
		  $(MAKE) -C $(ARCH_DIR)/test clean ;\
		  $(MAKE) -C $(ARCH_DIR)/test/nuse clean

ALL_OBJS=$(NUSE_OBJ) $(SIM_OBJ) $(OBJS) $(KERNEL_LIB) $(modules) $(all-obj-for-clean)
archclean:
	$(call if_changed,clean)

quiet_cmd_linknuse = LIBNUSE	$@
      # order of $(dpdkl_$(DPDK)) matters...
      cmd_linknuse = $(CC) -Wl,--whole-archive $(dpdkl_$(DPDK)) $(OBJS) $(NUSE_OBJ) $(LDFLAGS) $(LDFLAGS_NUSE) -o $@; \
		     ln -s -f $(NUSE_LIB) libnuse-linux.so

quiet_cmd_linksim = LIBSIM	$@
      cmd_linksim = $(CC) -Wl,--whole-archive $(OBJS) $(SIM_OBJ) $(LDFLAGS) $(LDFLAGS_SIM) -o $@; \
		    ln -s -f $(SIM_LIB) liblinux.so

$(NUSE_LIB): $(ARCH_DIR)/objs.mk $(CRC32TABLE) $(AUTOCONF) $(DPDK_OBJ) $(NUSE_OBJ) $(OBJS) $(ARCH_DIR)/linker.lds
	$(call if_changed,linknuse)

$(SIM_LIB): $(ARCH_DIR)/objs.mk $(CRC32TABLE) $(AUTOCONF) $(SIM_OBJ) $(OBJS) $(ARCH_DIR)/linker.lds
	$(call if_changed,linksim)

test:
	$(MAKE) -C $(ARCH_DIR)/test

.PHONY : clean depend

quiet_cmd_deps = DEPS  $@
      cmd_deps = set -e; $(CC) -MM -MT $(<:.c=.o) $(CFLAGS) $< > $@
.%.d:%.c $(srctree)/.config
	$(call if_changed,deps)

KBUILD_KCONFIG := arch/$(ARCH)/Kconfig
CLEAN_FILES := $(ARCH_DIR)/test/buildtop/source/quagga/lib/*.lo
