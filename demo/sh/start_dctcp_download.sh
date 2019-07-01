#!/bin/bash
DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A
declare -i num=$1

ssh $DCTCPSERVER "killall dl_server" 
ssh $DCTCPCLIENT "killall dl_client" 
if [[ $num > 0 ]]; then
	ssh $DCTCPSERVER "traffic_generator/dl_server/dl_server 5555" &
	ssh $DCTCPCLIENT "traffic_generator/dl_client/dl_client ${DCTCPSERVER} 5555 ${num} &" &
fi
