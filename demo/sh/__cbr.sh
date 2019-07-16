#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: $0 <rate>"
 	exit 65
fi

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"
declare -i rate=$1

do_ssh ${SERVER} 'killall iperf' &> /dev/null
do_ssh ${CLIENT} 'killall iperf' &> /dev/null
if [[ "$rate" > 0 ]]; then
    SSH_FLAGS="-f"
    do_ssh ${CLIENT}  "iperf -Ds"
    do_ssh ${SERVER}  "iperf -c ${CLIENT} -u -t 0 -b ${rate}m" &
fi
