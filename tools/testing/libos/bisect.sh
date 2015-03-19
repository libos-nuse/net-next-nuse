#!/bin/sh

git merge origin/nuse --no-commit
make clean ARCH=lib
make library ARCH=lib OPT=no
make test ARCH=lib ADD_PARAM=" -s dce-umip"
RET=$?
git reset --hard

exit $RET
