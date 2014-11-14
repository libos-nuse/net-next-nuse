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
 sudo NUSEDEV=eth0 nuse-eth0=192.168.209.39 ./nuse ping 192.168.209.1
```

where an environmental variable **nuse-(interface name)** indicates an IPv4 address for an interface under NUSE, instead of host OS's one.

and if you want to use netmap for network i/o, specify **NUSEVIF=NETMAP** as an env variable. The default interface will be raw socket based network i/o.

```
 sudo NUSEVIF=NETMAP NUSEDEV=eth0 nuse-eth0=192.168.209.39 ./nuse ping 192.168.209.1
```

And if you want to use tun/tap driver for network i/o, **NUSEVIF=TAP** is offered. When NUSEVIF is TAP, a name of interface attached to a nuse process is not restricted to existing interface names of host stack.

```
 sudo NUSEVIF=TAP NUSEDEV=eth0 nuse-eth0=192.168.209.39 ./nuse ping 192.168.209.1

 ~ host stack ~
 ifconfig nuse-eth0
 nuse-eth0 Link encap:Ethernet  HWaddr d6:d6:86:74:bf:5e  
           inet6 addr: fe80::d4d6:86ff:fe74:bf5e/64 Scope:Link
           UP BROADCAST RUNNING  MTU:1500  Metric:1
           RX packets:11 errors:0 dropped:0 overruns:0 frame:0
           TX packets:8 errors:0 dropped:0 overruns:0 carrier:0
           collisions:0 txqueuelen:500 
           RX bytes:462 (462.0 B)  TX bytes:648 (648.0 B)
```

How to set default route of nuse process.
```
 sudo NUSEVIF=TAP NUSEDEV=eth0 nuse-eth0=192.168.0.10 DEFAULTROUTE=192.168.0.1 ./nuse ping 172.16.0.1
```

And, iperf

```
 sudo NUSEDEV=eth0 nuse-eth0=192.168.209.39 ./nuse iperf -c 192.168.209.1 -u
```

should just work fine !

since the LD_PRELOAD with sudo technique requires additional copy and permission changes to the library, the script will automatically conduct such an operation.

## Tested platform
Fedora 19 64bits
Ubuntu 13.04 64bits
Ubuntu 14.04 64bits
