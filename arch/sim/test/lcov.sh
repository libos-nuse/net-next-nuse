#!/bin/sh

DIRS=`/bin/ls files-* -d`
PWD=`pwd`
CMD_OPT=""
if [ -z ${OUTDIR} ] ;then
    OUTDIR="./gcov-dce-html"
fi

for node in ${DIRS}
do

    SCAN_DIRS="-d ${node}/${PWD}/net-next-sim/net/ipv4" \
	"-d ${node}/${PWD}net-next-sim/net/ipv6 -d ${node}/${PWD}net-next-sim/net/core" \
	"-d ${node}/${PWD}net-next-sim/net/netlink" \
	"-d ${node}/${PWD}net-next-sim/net/packet" \
	"-d ${node}/${PWD}net-next-sim/net/xfrm"

#gcov *.gcda
find ../net-next-sim -name "*.gcno" |grep -v files- \
    | cpio -pud ${node}/${PWD}net-next-sim > /dev/null
../ns-3-dev/utils/lcov/lcov -q -c ${SCAN_DIRS} \
    -b ${PWD}net-next-sim/ -o dce-run-${node}.info
CMD_OPT="$CMD_OPT"" -a dce-run-${node}.info"

done

../ns-3-dev/utils/lcov/lcov -q ${CMD_OPT} -o dce-run.info
../ns-3-dev/utils/lcov/genhtml -q --legend -o ${OUTDIR} dce-run.info > /dev/null

rm -f dce-run-files-*.info
