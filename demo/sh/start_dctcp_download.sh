#!/bin/bash
DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A
declare -i num=$1
HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh $DCTCPSERVER "killall dl_server" 
do_ssh $DCTCPCLIENT "killall dl_client" 
if [[ $num > 0 ]]; then
    do_ssh $DCTCPSERVER "traffic_generator/dl_server/dl_server 5555" &
    do_ssh $DCTCPCLIENT "traffic_generator/dl_client/dl_client ${DCTCPSERVER} 5555 ${num} &" &
fi
