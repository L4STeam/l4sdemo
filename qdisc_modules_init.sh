#!/bin/bash
#set -e

if [ ${PWD##*/} = "l4sdemo" ]; then
    HERE=$(realpath $(dirname $0))
else
    HERE=$(realpath $(dirname $(pwd)))
fi

source "$HERE/environment.sh"

REV=$(uname -r | awk -F '.' '{ printf "%d.%d", $1, $2 }')

export EXTRA_CFLAGS+="-I. -I ${HERE}/common -DIS_TESTBED=1"

# Preemptively attempt to unload dualpi2
sudo tc qdisc del root dev $IFACE || true

mod_dir=${HERE}/kernel_modules/${REV}
if [ ! -d $mod_dir ]; then
    echo ""
    echo "Cannot find kernel modules matching the running kernel!"
    exit 1
fi

for mod in $mod_dir/*; do
    varname=$(make -qp -f ${mod}/Makefile | awk '/TARGET :=/ { print $3; }')
    if [ "$REV" = "5.3" ]; then
        (cd $mod && \
            make "CFLAGS_${varname}.o='${EXTRA_CFLAGS}'" && \
            sudo make unload \
        && sudo make load)
    else
        
        (cd $mod && \
            make -e "CFLAGS_${varname}.o='${EXTRA_CFLAGS}'" && \
            sudo make unload \
        && sudo make load)
    fi
done