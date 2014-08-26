NUSE (Network Stack in Userspace) [![Build Status](https://travis-ci.org/thehajime/net-next-nuse.png)](https://travis-ci.org/thehajime/net-next-nuse)
===============================


## NOTE
 because this is really alpha version, IP address and interface name are 'hard-coded': you need to modify by yourself at this moment.

## HOWTO

```
 make defconfig ARCH=sim
 make library ARCH=sim OPT=no
```

for a build with netmap,

```
make library ARCH=sim OPT=no NETMAP=yes
```

you should see libnuse-linux.so.

Then, a wrapper script called **nuse** takes your application running with NUSE.

```
 sudo ./nuse iperf -c 192.168.209.1 -u
```

since the LD_PRELOAD with sudo technique requires additional copy and permission changes to the library, the script will automatically conduct such an operation.
