#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: ./cbr_cubic.sh <rate>"
 	exit 65
fi

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

rate=$1

do_ssh ${RENOSERVER} 'killall iperf'
if [ "$rate" != "0" ]; then
    do_ssh ${RENOSERVER}  "iperf -c ${RENOCLIENT} -u -t 500 -b ${rate}m &" &
fi
