#!/bin/sh

set -e
#set -x
export LD_LOG=symbol-fail
#VERBOSE="-v"
VALGRIND=""
FAULT_INJECTION=""

if [ "$1" = "-g" ] ; then
 VALGRIND="-g"
elif [ "$1" = "-f" ] ; then
 FAULT_INJECTION="-f"
fi

# FIXME
export NS_ATTRIBUTE_DEFAULT='ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[];ns3::TaskManager::FiberManagerType=UcontextFiberManager' 

cd buildtop/source/dce
LD_LIBRARY_PATH=${srctree} ./test.py -z test-runner-vdl -n ${VALGRIND} ${FAULT_INJECTION} ${VERBOSE}
