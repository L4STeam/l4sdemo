#!/bin/bash

HERE=$(realpath $(dirname $0))
DEST="${HERE}/../iproute2-l4s"
TCDIR="${HERE}/tc"

echo "Building iproute2"

rm -rf "$DEST"
git clone \
    --depth 1 --single-branch \
    -b $(uname -r | awk -F '.' '{ printf "v%d.%d.0", $1, $2 }')
    git://git.kernel.org/pub/scm/network/iproute2/iproute2.git "$DEST"

for qdisc in *.c; do
    cp "$qdisc" "${TCDIR}"
    sed -i "/^TCMODULES :=/a TCMODULES += ${qdisc/%.c/.o}" "${TCDIR}/Makefile"
done

cat "${HERE}/../kernel_modules/sch_dualpi2/compat-pkt_sched.h" >> "${DEST}/include/uapi/linux/pkt_sched.h"

pushd "${TCDIR}"
./configure
make -j$(nproc)
popd
