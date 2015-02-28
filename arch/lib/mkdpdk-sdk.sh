#!/bin/sh

cd $1

export CPP="cc -E"
CPP="cc -E" make CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=y T=$(uname -m)-native-linuxapp-gcc config V=1
#make SRCARCH=x86 CONFIG_RTE_BUILD_COMBINE_LIBS=y EXTRA_CFLAGS="-fPIC -g"
