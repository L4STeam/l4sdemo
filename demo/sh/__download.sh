#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

declare -i num=$1

do_ssh $SERVER "killall dl_server" &> /dev/null
do_ssh $CLIENT "killall dl_client" &> /dev/null
if [[ $num > 0 ]]; then
	echo "starting $num flows between $SERVER and $CLIENT [$DL_PORT]"
    # Start the following programs in background
    SSH_FLAGS="-f"
    do_ssh $SERVER "traffic_generator/dl_server/dl_server ${DATAPATH_SERVER} ${DL_PORT}" &
    do_ssh $CLIENT "traffic_generator/dl_client/dl_client ${DATAPATH_SERVER} ${DL_PORT} ${num}" &
fi
