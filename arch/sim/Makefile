ARCH_DIR := arch/sim
SRCDIR=$(dir $(firstword $(MAKEFILE_LIST)))

CC = gcc
GCCVERSIONGTEQ48 := $(shell expr `gcc -dumpversion` \>= 4.8)
ifeq "$(GCCVERSIONGTEQ48)" "1"
   NO_TREE_LOOP_OPT += -fno-tree-loop-distribute-patterns
endif


SIM_SRC=\
fs.c random.c sim.c softirq.c time.c \
glue.c sim-device.c sysctl.c timer.c \
hrtimer.c sched.c sim-socket.c sysfs.c workqueue.c \
print.c security.c slab.c tasklet.c \
proc.c seq.c socket.c tasklet-hrtimer.c \
cred.c pid.c modules.c filemap.c splice.c vmscan.c \
dcache.c super.c inode.c

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
KERNEL_LIB=libsim-linux$(KERNELVERSION).so
SIM_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(SIM_SRC))))
CFLAGS+= \
 $(opt_$(OPT)) -g3 -D__KERNEL__ -Wall -Wstrict-prototypes -Wno-trigraphs \
 -fno-inline -iwithprefix $(srctree)/include -DKBUILD_BASENAME=\"clnt\" \
 -fno-strict-aliasing -fno-common -fno-delete-null-pointer-checks \
 -fno-stack-protector -Wno-unused -Wno-pointer-sign \
 -DKBUILD_MODNAME=\"nsc\" -DMODVERSIONS -DEXPORT_SYMTAB \
 -include $(srctree)/include/linux/kconfig.h \
 -U__FreeBSD__ -D__linux__=1 -Dlinux=1 -D__linux=1 \
 -I$(ARCH_DIR)/include -I$(srctree)/include \
 -I$(ARCH_DIR) -I$(srctree) -I$(srctree) -I$(srctree)/include/uapi \
 $(PIC_CFLAGS) -D_DEBUG $(cov_$(COV))

include $(ARCH_DIR)/processor.mk

ifeq ($(PROCESSOR_SIZE),64)
CFLAGS+= -DCONFIG_64BIT
endif

LDFLAGS += -shared -nodefaultlibs -g3 -Wl,-O1 -Wl,-T$(ARCH_DIR)/linker.lds $(covl_$(COV))

modules:=
all-obj-for-clean:=

all: library modules

-include $(ARCH_DIR)/objs.mk

AUTOCONF=$(ARCH_DIR)/autoconf.h
# note: the directory order below matters to ensure that we match the kernel order
dirs=kernel/ kernel/rcu/ kernel/locking/ mm/ fs/ fs/proc/ crypto/ lib/ drivers/base/ drivers/net/ net/
empty:=
space:= $(empty) $(empty)
colon:= :
comma= ,
kernel/_to_keep=notifier.o params.o time.o sysctl.o \
rwsem.o semaphore.o kfifo.o
kernel/rcu_to_keep=rcu/srcu.o rcu/pdate.o rcu/tiny.o
kernel/locking_to_keep=locking/mutex.o
mm/_to_keep=util.o
crypto/_to_keep=aead.o ahash.o shash.o api.o algapi.o cipher.o compress.o proc.o \
crc32c_generic.o
drivers/base/_to_keep=class.o core.o bus.o dd.o driver.o devres.o module.o
drivers/net/_to_keep=loopback.o
lib/_to_keep=klist.o kobject.o kref.o hweight.o int_sqrt.o checksum.o \
find_last_bit.o find_next_bit.o bitmap.o nlattr.o idr.o libcrc32c.o \
ctype.o string.o kasprintf.o rbtree.o sha1.o textsearch.o vsprintf.o \
rwsem-spinlock.o scatterlist.o ratelimit.o hexdump.o dec_and_lock.o \
div64.o dynamic_queue_limits.o md5.o kstrtox.o iovec.o lockref.o crc32.o
fs/_to_keep=read_write.o libfs.o namei.o
fs/proc/_to_keep=proc_sysctl.o


quiet_cmd_objsmk = OBJS-MK   $@
      cmd_objsmk = \
	for i in 1; do \
	$(foreach d,$(dirs), \
           $(MAKE) -i -s -f $< srcdir=$(srctree)/$(d) \
	    objdir=$(srctree)/$(d) \
            config=$(srctree)/.config \
	    to_keep=$(subst $(space),$(colon),$($(d)_to_keep)) print;) \
	done > $@

$(ARCH_DIR)/objs.mk: $(ARCH_DIR)/Makefile.print $(srctree)/.config
	$(call if_changed,objsmk)

-include $(srctree)/.config

$(AUTOCONF): $(ARCH_DIR)/generate-autoconf.py $(srctree)/.config $(ARCH_DIR)/timeconst.h
	$(ARCH_DIR)/generate-autoconf.py $(srctree)/.config > $@
$(ARCH_DIR)/timeconst.h: $(srctree)/.config
	echo "hz=$(CONFIG_HZ)" > $(srctree)/arch/sim/hz.bc
	bc $(srctree)/arch/sim/hz.bc kernel/timeconst.bc > $@
$(ARCH_DIR)/linker.lds: $(ARCH_DIR)/generate-linker-script.py
	ld -shared --verbose | ./$^ > $@

# from lib/Makefile
CRC32TABLE = $(ARCH_DIR)/crc32table.h
hostprogs-y	:= $(srctree)/lib/gen_crc32table
clean-files	:= crc32table.h

$(srctree)/lib/gen_crc32table: $(srctree)/lib/gen_crc32table.c
	cc $^ -o $@

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(CRC32TABLE): $(srctree)/lib/gen_crc32table
	$(call if_changed,crc32)

KERNEL_BUILTIN=$(addprefix $(srctree)/,$(addsuffix builtin.o,$(dirs)))
OBJS=$(SIM_OBJ) $(foreach builtin,$(KERNEL_BUILTIN),$(if $($(builtin)),$($(builtin))))

quiet_cmd_cc = CC   $@
      cmd_cc = 	mkdir -p $(dir $@);	\
		$(CC) $(CFLAGS) -c $^ -o $@
quiet_cmd_linkko = KO   $@
      cmd_linkko = $(CC) -shared -o $@ -nostdlib $^
quiet_cmd_builtin = BUILTIN   $@
      cmd_builtin = mkdir -p $(dir $(srctree)/$@); if test -n "$($(srctree)/$@)"; then for f in $($(srctree)/$@); \
		    do $(AR) Tcru $@ $$f; done; else $(AR) Tcru $@; fi

%/builtin.o:
	$(call if_changed,builtin)
%.ko:%.o
	$(call if_changed,linkko)
%.o:%.c
	$(call if_changed,cc)

library: $(KERNEL_LIB)
modules: $(modules) 

install: modules library

install-dir:

.patch.ts: $(ARCH_DIR)/kernel.patch
	CWD=`pwd` && cd $(srctree) && patch -p1 < $(ARCH_DIR)/kernel$(KERNEL_VERSION).patch && cd $$CWD && touch $(ARCH_DIR)/.patch.ts
unpatch: 
	CWD=`pwd` && cd $(srctree) && git reset --hard $(KERNEL_VERSION) && cd $$CWD && rm -f $(ARCH_DIR)/.patch.ts

quiet_cmd_clean = CLEAN   $@
      cmd_clean = for f in $(foreach m,$(modules),$($(m))) ; do rm -f $$f 2>/dev/null; done ; \
		  for f in $(OBJS) $(KERNEL_LIB) $(modules) $(all-obj-for-clean); do rm -f $$f; done 2>/dev/null ;\
		  rm -rf $(ARCH_DIR)/linker.lds $(AUTOCONF) $(ARCH_DIR)/objs.mk $(ARCH_DIR)/timeconst.h 2>/dev/null ;\
		  $(MAKE) -C $(ARCH_DIR)/test clean

ALL_OBJS=$(OBJS) $(KERNEL_LIB) $(modules) $(all-obj-for-clean)
archclean:
	$(call if_changed,clean)

quiet_cmd_linklib = LIB   $@
      cmd_linklib = $(CC) $(LDFLAGS) -Wl,--whole-archive $(OBJS) -o $@; \
		    ln -s -f $(KERNEL_LIB) liblinux.so

$(KERNEL_LIB): $(ARCH_DIR)/objs.mk $(CRC32TABLE) $(AUTOCONF) $(OBJS) $(ARCH_DIR)/linker.lds
	$(call if_changed,linklib)

test:
	$(MAKE) -C $(ARCH_DIR)/test

.PHONY : clean

KBUILD_KCONFIG := arch/$(ARCH)/Kconfig
CLEAN_FILES := $(ARCH_DIR)/test/buildtop/source/quagga/lib/*.lo
