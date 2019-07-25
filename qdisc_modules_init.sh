#!/bin/bash
#set -e

HERE=$(realpath $(dirname $0))

source "$HERE/environment.sh"

REV=$(uname -r | awk -F '.' '{ printf "%d.%d", $1, $2 }')

export EXTRA_CFLAGS+="-I. -I ${HERE}/common -DIS_TESTBED=1"

# Preemptively attempt to unload dualpi2
sudo tc qdisc del root dev $IFACE || true

mod_dir=${HERE}/kernel_modules/${REV}
if [ -d $mod_dir ]; then
    echo ""
    echo "Cannot find kernel modules matching the running kernel!"
    exit 1
fi

for mod in $mod_dir/*; do
	varname=$(make -qp -f ${mod}/Makefile | awk '/TARGET :=/ { print $3; }')
	(cd $mod && \
        make -e "CFLAGS_${varname}.o='${EXTRA_CFLAGS}'" && \
        sudo make unload \
        && sudo make load)
done
