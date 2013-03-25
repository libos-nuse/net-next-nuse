
all: test

bake:
	hg clone http://code.nsnam.org/bake

testbin: bake
	@mkdir -p buildtop/build/bin_dce
	cd buildtop ; \
	../bake/bake.py configure -c ../bakeconf.xml -e dce-umip ; \
	../bake/bake.py download ; \
	../bake/bake.py update ; \
	../bake/bake.py build 

test:
	@./test.sh

test-valgrind:
	@./test.sh -g

test-fault-injection:
	@./test.sh -f

clean:
#	@rm -rf buildtop
	@rm -f *.pcap
	@rm -rf files-*
	@rm -f exitprocs
	@rm -f core
