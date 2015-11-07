ADD_PARAM?=

all: test

bake:
	hg clone http://code.nsnam.org/bake

check_pkgs:
	@./bake/bake.py check | grep Bazaar | grep OK || (echo "bzr is missing" && ./bake/bake.py check)
	@./bake/bake.py check | grep autoreconf | grep OK || (echo "autotools is missing" && ./bake/bake.py check && exit 1)

testbin: bake check_pkgs
	@cp ../../../arch/lib/tools/bakeconf-linux.xml bake/bakeconf.xml
	@mkdir -p buildtop/build/bin_dce
	cd buildtop ; \
	../bake/bake.py configure -e dce-linux-inkernel $(BAKECONF_PARAMS)
	cd buildtop ; \
	../bake/bake.py show --enabledTree | grep -v  -E "pygoocanvas|graphviz|python-dev" | grep Missing && (echo "required packages are missing") || echo ""
	cd buildtop ; \
	../bake/bake.py download ; \
	../bake/bake.py update ; \
	../bake/bake.py build

test:
	@./dce-test.sh ADD_PARAM=$(ADD_PARAM)

test-valgrind:
	@./dce-test.sh -g ADD_PARAM=$(ADD_PARAM)

test-fault-injection:
	@./dce-test.sh -f ADD_PARAM=$(ADD_PARAM)

clean:
#	@rm -rf buildtop
	@rm -f *.pcap
	@rm -rf files-*
	@rm -f exitprocs
	@rm -f core
