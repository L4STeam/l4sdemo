#!/bin/bash

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

declare -i num=$1

#./killall_dctcp.sh
ssh $RENOSERVER "killall dl_server"
ssh $RENOCLIENT "killall dl_client"
if [[ $num > 0 ]]; then
	echo "starting $num"
        sleep 0.2
        ssh $RENOSERVER "traffic_generator/dl_server/dl_server 5555" &
	sleep 1
        ssh $RENOCLIENT "traffic_generator/dl_client/dl_client ${RENOSERVER} 5555 ${num} &" &
fi

