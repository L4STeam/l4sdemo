#!/bin/bash
DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A
declare -i num=$1

#./killall_dctcp.sh
#ssh $DCTCPSERVER "scp Downloads/demo/bigfile ${DCTCPCLIENT}:Downloads/." &
ssh $DCTCPSERVER "killall dl_server" 
ssh $DCTCPCLIENT "killall dl_client" 
if [[ $num > 0 ]]; then
	sleep 0.2
	ssh $DCTCPSERVER "dual-queue-aqm/traffic_generator/dl_server/dl_server 5555" &
	sleep 1
	ssh $DCTCPCLIENT "dual-queue-aqm/traffic_generator/dl_client/dl_client ${DCTCPSERVER} 5555 ${num} &" &
fi
