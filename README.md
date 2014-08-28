NUSE (Network Stack in Userspace) [![Build Status](https://travis-ci.org/thehajime/net-next-nuse.png)](https://travis-ci.org/thehajime/net-next-nuse)
===============================


# HOWTO
## Build

```
 make defconfig ARCH=sim
 make library ARCH=sim OPT=no
```

for a build with netmap,

```
make library ARCH=sim OPT=no NETMAP=yes
```

you should see libnuse-linux.so.

## Run

Then, a wrapper script called **nuse** takes your application running with NUSE.

```
 sudo nuse-eth0=192.168.209.39 ./nuse ping 192.168.209.1
```

where **nuse-(interface name)** indicates an IPv4 address for an interface under NUSE, instead of host OS's one.


And, iperf

```
 sudo nuse-eth0=192.168.209.39 ./nuse iperf -c 192.168.209.1 -u
```

should just work fine !

since the LD_PRELOAD with sudo technique requires additional copy and permission changes to the library, the script will automatically conduct such an operation.
