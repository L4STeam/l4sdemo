#!/bin/bash

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

TESTBED=demo
RENOPORT="11001"
PATH_TO_TRAFFIC_GENERATOR="traffic_generator"

declare -i mit_r=$1*1000
link=$2

ssh $RENOCLIENT 'killall http_clients_itime'
ssh $RENOSERVER 'killall run_httpserver'
if (( "$mit_r" != "0" )); then
	ssh $RENOSERVER "${PATH_TO_TRAFFIC_GENERATOR}/http_server/run_httpserver 10002 rs2.txt" &
fi
ssh $RENOCLIENT 'killall http_clients_itime'
if (( "$mit_r" != "0" )); then
#	echo ssh $RENOCLIENT "${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${RENOSERVER} 10002 rit${mit_r}_2.txt ${AQMNODE} ${RENOPORT}"
	ssh $RENOCLIENT "${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${RENOSERVER} 10002 rit${mit_r}_2.txt $link ${AQMNODE} ${RENOPORT}" &
fi

