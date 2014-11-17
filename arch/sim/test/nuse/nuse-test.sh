#!/bin/bash

IFNAME=`ip route |grep default | awk '{print $5}'`
GW=`ip route |grep default | awk '{print $3}'`
#XXX
IPADDR=`echo $GW | sed -r "s/([0-9]+\.[0-9]+\.[0-9]+\.)([0-9]+)$/\1\`expr \2 + 10\`/"`
sudo NUSEDEV=${IFNAME} nuse-${IFNAME}=${IPADDR} ./nuse ping ${GW} -c 1
