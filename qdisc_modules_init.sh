#!/bin/bash
set -e

export EXTRA_CFLAGS+="-I. -I aqms -DIS_TESTBED=1"

for aqm in $(ls aqms); do
	varname="CFLAGS_$(make -qp -f aqms/${aqm}/Makefile | grep 'TARGET :=' | awk '{ print $3; }').o"
	(cd aqms/${aqm} && make -e "$varname='$EXTRA_CFLAGS'" && sudo make unload && sudo make load)
done
