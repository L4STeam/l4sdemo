#!/bin/bash

HERE=$(realpath $(dirname $0))
DEST="${HERE}/../iproute2-l4s"
TCDIR="${DEST}/tc"

QDISC_NAME=fq_codel_tst
FNAME="${TCDIR}/q_fq_codel.c"

if ! grep "$QDISC_NAME" "$FNAME"; then
    cat >> "$FNAME" << EOF
struct qdisc_util ${QDISC_NAME}_qdisc_util = {
	.id		= "${QDISC_NAME}",
	.parse_qopt	= fq_codel_parse_opt,
	.print_qopt	= fq_codel_print_opt,
	.print_xstats	= fq_codel_print_xstats,
};
EOF
fi
