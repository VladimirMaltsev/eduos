#!/bin/sh

. $TESTDIR/lib.sh

make_eduos

check_out $CDIR/1 ./image ./test/t/cat/rootfs
