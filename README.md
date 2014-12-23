A library operating system version of Linux kernel
==================================================
NUSE [![Build Status](https://travis-ci.org/libos-nuse/net-next-nuse.png)](https://travis-ci.org/libos-nuse/net-next-nuse)  
DCE [![Build Status](https://travis-ci.org/direct-code-execution/net-next-sim.png)](https://travis-ci.org/direct-code-execution/net-next-sim)

<!--
[![Code Health](https://landscape.io/github/libos-nuse/net-next-nuse/nuse/landscape.svg)](https://landscape.io/github/libos-nuse/net-next-nuse/nuse)
-->


## What is this
This is a library operating system (LibOS) version of Linux kernel,
which will benefit in the couple of situations like:


* operating system personalization
* full featured network stack for kernel-bypass technology (a.k.a. a high-speed packet I/O mechanism) like
Intel DPDK, netmap, etc
* testing network stack in a complex scenario.


For more detail, please refer the document.

https://github.com/libos-nuse/net-next-nuse/tree/nuse/Documentation/virtual/libos-howto.txt

## Applications
Right now, we have 2 sub-projects of this LibOS.

- Network Stack in Userspace (NUSE)  
_NUSE_ allows us to use Linux network stack as a library which any applications can directory use by linking the library.
Each application has its own network stack so, it provides an instant virtualized environment apart from a host operating system.
- Direct Code Execution (DCE)
_DCE_ provides network simulator integration with Linux kernel so that any Linux implemented network protocols are for a protocol under investigate. Check http://www.nsnam.org/overview/projects/direct-code-execution/ for more detail about DCE.


## Quick start
* for NUSE  
https://github.com/libos-nuse/net-next-nuse/wiki/Quick-Start
* for DCE  
http://direct-code-execution.github.io/net-next-sim/
