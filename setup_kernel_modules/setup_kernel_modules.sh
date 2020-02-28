#!/bin/bash
#set -e

if [ ${PWD##*/} = "l4sdemo" ]; then
    HERE=$(realpath $(dirname $0))    
else
    HERE=$(realpath $(dirname $(pwd)))
fi

REV=$(uname -r | awk -F '.' '{ printf "%d.%d", $1, $2 }')

export EXTRA_CFLAGS+="-I. -I ${HERE}/common -DIS_TESTBED=1"

# The interface facing the senders
IFACE_SENDERS="enp101s0f0"
# The interface facing the coordinator
IFACE_COORDINATOR="enp101s0f1"

# Preemptively attempt to unload dualpi2
for device in $IFACE_SENDERS $IFACE_COORDINATOR; do
    sudo tc qdisc del root dev $device || true
done

mod_dir=${HERE}/kernel_modules/${REV}
if [ ! -d $mod_dir ]; then
    echo ""
    echo "Cannot find kernel modules matching the running kernel!"
    exit 1
fi

for mod in $mod_dir/*; do
    varname=$(make -qpv -f ${mod}/Makefile | awk '/TARGET :=/ { print $3; }')
    (cd $mod && \
        make -e "CFLAGS_${varname}.o='${EXTRA_CFLAGS}'" && \
        sudo make unload \
    && sudo make load)
done

make -e "CFLAGS_$(make -qpv -f Makefile | awk '/TARGET :=/ { print $3; }').o='-I. -I /home/$(whoami)/l4sdemo/common -DIS_TESTBED=1'" && sudo make unload && sudo make load
