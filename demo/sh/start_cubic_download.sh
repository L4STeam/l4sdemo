#!/bin/bash

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

declare -i num=$1

do_ssh $RENOSERVER "killall dl_server"
do_ssh $RENOCLIENT "killall dl_client"
if [[ $num > 0 ]]; then
	echo "starting $num flows"
    do_ssh $RENOSERVER "traffic_generator/dl_server/dl_server 5555" &
    do_ssh $RENOCLIENT "traffic_generator/dl_client/dl_client ${RENOSERVER} 5555 ${num} &" &
fi
