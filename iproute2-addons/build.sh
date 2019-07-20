#!/bin/bash

HERE=$(realpath $(dirname $0))
DEST="${HERE}/../iproute2-l4s"
TCDIR="${DEST}/tc"

echo "Building iproute2"

if [ ! -d "$DEST" ]; then
git clone \
    --depth 1 --single-branch \
    -b $(uname -r | awk -F '.' '{ printf "v%d.%d.0", $1, $2 }') \
    git://git.kernel.org/pub/scm/network/iproute2/iproute2.git "$DEST"
fi
pktsch_h="${DEST}/include/uapi/linux/pkt_sched.h"
if [ ! -f "$pktsch_h" ]; then
    pktsch_h="${DEST}/include/linux/pkt_sched.h"
fi
pushd "$DEST"
git checkout "include/"
popd
cat >> "$pktsch_h" << EOF
#ifndef DUALPI2_DEV
#include "${HERE}/../kernel_modules/sch_dualpi2/compat-pkt_sched.h"
#else
#include "${HERE}/../kernel_modules/sch_dualpi2_dev/compat-pkt_sched.h"
#endif
EOF
for qdisc in ${HERE}/*.c; do
    qdisc_o="$(basename $qdisc)"
    qdisc_o="${qdisc_o/%.c/.o}"
    if ! grep "TCMODULES +=${qdisc_o}" "${TCDIR}/Makefile"; then
        sed -i "/^TCMODULES :=/a TCMODULES += ${qdisc_o}" "${TCDIR}/Makefile"
    fi
    cp "$qdisc" "${TCDIR}"
done

"${HERE}/patch_fq_codel.sh"

pushd "${DEST}"
./configure
make -j$(nproc)
popd
