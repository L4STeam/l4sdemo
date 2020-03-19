#!/bin/bash
#set -e

HERE=$(realpath $(dirname $0))

REV=$(uname -r | awk -F '.' '{ printf "%d.%d", $1, $2 }')

REV=$(uname -r | awk -F '.' '{ printf "%d.%d", $1, $2 }')

export EXTRA_CFLAGS+="-I. -I ${HERE}/common -DIS_TESTBED=1"

# Preemptively attempt to unload dualpi2
# Traffic control settings, apply to queueing discipline (qdisc), delete from user id root, remove the (dev)ice on interface $IFACE.
# THAT IS: Remove the queueing discipline from the interface $IFACE.

sudo tc qdisc del root dev $IFACE || true

mod_dir=${HERE}/kernel_modules/${REV}
if [ ! -d $mod_dir ]; then
    echo ""
    echo "Cannot find kernel modules matching the running kernel!"
    exit 1
fi

for mod in $mod_dir/*; do
    varname=$(make -qpv -f ${mod}/Makefile | awk '/TARGET :=/ { print $3; }')

    # If we use the -e option in `make`, the environment variables will be overwritten which somehow causes Kbuild to look for a Makefile inside a /script in the kernel_modules directory. This seemed to happen only on kernel version 5.3.*, so we removed the -e option altogether.
    if [ "$REV" = "5.3" ]; then
        (cd $mod &&
            make "CFLAGS_${varname}.o='${EXTRA_CFLAGS}'" &&
            sudo make unload &&
            sudo make load)
    else
        (cd $mod &&
            make -e "CFLAGS_${varname}.o='${EXTRA_CFLAGS}'" &&
            sudo make unload &&
            sudo make load)
    fi
done

# On kernel version 5.3.* `make` would complain about a missing Makefile if this code is executed too after inserting tcp_dctp.
if [ "$REV" != "5.3" ]; then
    make -e "CFLAGS_$(make -qpv -f Makefile | awk '/TARGET :=/ { print $3; }').o='-I. -I /home/$(whoami)/l4sdemo/common -DIS_TESTBED=1'" && sudo make unload && sudo make load
fi
