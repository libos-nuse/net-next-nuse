ADD_PARAM?=

all: test

bake:
	hg clone http://code.nsnam.org/bake

check_pkgs:
	@./bake/bake.py check | grep Bazaar | grep OK || (echo "bzr is missing" && ./bake/bake.py check)
	@./bake/bake.py check | grep Autotools | grep OK || (echo "autotools is missing" && ./bake/bake.py check && exit 1)

testbin: bake check_pkgs
	@mkdir -p buildtop/build/bin_dce
	cd buildtop ; \
	../bake/bake.py configure -e dce-umip-inkernel 
	cd buildtop ; \
	../bake/bake.py show --enabledTree || (echo "required packages are missing" && exit 1)
	cd buildtop ; \
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
