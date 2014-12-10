NUSE (Network Stack in Userspace) [![Build Status](https://travis-ci.org/libos-nuse/net-next-nuse.png)](https://travis-ci.org/libos-nuse/net-next-nuse) [![Code Health](https://landscape.io/github/libos-nuse/net-next-nuse/nuse/landscape.svg)](https://landscape.io/github/libos-nuse/net-next-nuse/nuse)
===============================


# HOWTO
## Build

```
 make defconfig ARCH=lib
 make library ARCH=lib OPT=no
```

for a build with netmap,

```
make library ARCH=lib OPT=no NETMAP=yes
```

you should see libnuse-linux.so.

## Build with Intel DPDK

You need to build dpdk sdk first.

```
make dpdk-sdk ARCH=lib DPDK=yes
```

then build with the SDK for DPDK channel of NUSE.

```
 make defconfig ARCH=lib
 make library ARCH=lib OPT=no DPDK=yes
```

Additional DPDK configuration is needed, hugepage setup, make an interface DPDK mode.

```
 ./dpdk/tools/setup.sh
 sudo insmod dpdk/build/kmod/igb_uio.ko
 sudo ./dpdk/tools/dpdk_nic_bind.py -b igb_uio 0000:02:01.0
```

## Run

At first, please write a configuration file for nuse **nuse.conf**.
Example of nuse.conf is shown below.

```
# Interface definition.
interface eth0
        address 192.168.0.10
        netmask 255.255.255.0
        macaddr 00:01:01:01:01:01
        viftype TAP

interface p1p1
        address 172.16.0.1
        netmask 255.255.255.0
        macaddr 00:01:01:01:01:02

# route entry definition.
route
        network 0.0.0.0
        netmask 0.0.0.0
        gateway 192.168.0.1
```

When viftype is TAP, the interface name attached to nuse process is
not restricted to be same as physical interfaces of host
stack. However, viftype RAW, NETMAP, and DPDK require an interface name
must be same as a physical interface of host stack.

The default interface will be raw socket based network i/o.


Then, a wrapper script called **nuse** takes your application running with NUSE.

```
 sudo NUSECONF=nuse.conf ./nuse ping 172.16.0.2
```

And, iperf

```
 sudo NUSECONF=nuse.conf ./nuse iperf -c 192.168.209.1 -u
```

should just work fine !

since the LD_PRELOAD with sudo technique requires additional copy and permission changes to the library, the script will automatically conduct such an operation.


### Run with DPDK

If you want to use nuse with dpdk, interface names of dpdk on nuse.conf must
follow the **dpdk%d** format.  The digit X of _dpdkX_ indicates the
dpdk port number. An example is shown below.

```
nuse1:net-next-nuse % cat nuse-dpdk.conf
interface dpdk0
	address 172.16.0.1
	netmask 255.255.255.0
	macaddr 00:01:01:01:01:01
	viftype DPDK

interface dpdk1
	address 172.16.1.1
	netmask 255.255.255.0
	macaddr 00:01:01:01:01:02
	viftype DPDK


nuse1:net-next-nuse % ./dpdk/tools/dpdk_nic_bind.py --status

Network devices using DPDK-compatible driver
============================================
0000:0a:00.0 'Ethernet 10G 2P X520 Adapter' drv=igb_uio unused=
0000:0a:00.1 'Ethernet 10G 2P X520 Adapter' drv=igb_uio unused=

~ snip ~
```

This set up means
"use PCI 0000:0a:00.0 as dpdk0 and PCI 0000:0a:00.1 as dpdk1".


## Tested platform
- Fedora 19 64bits
- Ubuntu 13.04 64bits
- Ubuntu 14.04 64bits
