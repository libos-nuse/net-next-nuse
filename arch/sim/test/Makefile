ADD_PARAM?=

all: test

bake:
	hg clone http://code.nsnam.org/bake

testbin: bake
	@mkdir -p buildtop/build/bin_dce
	cd buildtop ; \
	../bake/bake.py configure -e dce-umip-inkernel ; \
	../bake/bake.py download ; \
	../bake/bake.py update ; \
	../bake/bake.py build 

test:
	@./test.sh ADD_PARAM=$(ADD_PARAM)

test-valgrind:
	@./test.sh -g ADD_PARAM=$(ADD_PARAM)

test-fault-injection:
	@./test.sh -f ADD_PARAM=$(ADD_PARAM)

clean:
#	@rm -rf buildtop
	@rm -f *.pcap
	@rm -rf files-*
	@rm -f exitprocs
	@rm -f core
