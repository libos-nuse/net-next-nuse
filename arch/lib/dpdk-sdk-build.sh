#!/bin/sh

git submodule init
git submodule update dpdk

: ${RTE_SDK:=$(pwd)/dpdk}
: ${RTE_TARGET:=build}
export RTE_SDK
export RTE_TARGET

set -e
( cd dpdk ; make CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=y \
  T=$(uname -m)-native-linuxapp-gcc config &&  \
  make SRCARCH=x86 CONFIG_RTE_BUILD_COMBINE_LIBS=y EXTRA_CFLAGS="-fPIC -g" \
  || die dpdk build failed )

ls ${RTE_SDK}/${RTE_TARGET}/lib
