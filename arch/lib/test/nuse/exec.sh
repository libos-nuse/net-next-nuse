LD_PRELOAD=liblinux.so ./arch/sim/test/nuse/udp-client-native 192.168.209.1
# you need to chmod 4755 to iperf
LD_PRELOAD=liblinux.so   ../../build/bin/iperf -s -u -p 2000
# tcp as well
LD_PRELOAD=liblinux.so  ../../build/bin/iperf -c 192.168.209.1 -p 2000
