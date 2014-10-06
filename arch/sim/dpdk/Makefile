RTE_SDK := $(PWD)/dpdk
export RTE_SDK

RTE_TARGET ?= build

include $(RTE_SDK)/mk/rte.vars.mk

#
# directory list
#
LIB = libnuse-dpdk.a

SRCS-y := ../nuse-vif-dpdk.c

CFLAGS += -I../../include -I../../../../include/ -O0 -g -fPIC 
#CFLAGS += $(WERROR_FLAGS)
DEPDIRS-y += lib/librte_eal lib/librte_ethdev lib/librte_pmd_e1000

include $(RTE_SDK)/mk/rte.extlib.mk
