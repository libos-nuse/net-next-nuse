#!/bin/sh

set -e
#set -x
# LD_LOG=symbol-fail
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
LD_LIBRARY_PATH=${srctree} ./test.py -z ns3test-dce-vdl -n ${VALGRIND} ${FAULT_INJECTION}
cd ..
cd dce-quagga
LD_LIBRARY_PATH=${srctree} ../dce/test.py -n -z ns3test-dce-quagga-vdl ${VALGRIND} ${FAULT_INJECTION}
cd ..
cd dce-umip
LD_LIBRARY_PATH=${srctree} ../dce/test.py -n -z ns3test-dce-umip-vdl ${VALGRIND} ${FAULT_INJECTION}
cd ..
