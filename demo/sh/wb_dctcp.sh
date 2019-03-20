#!/bin/bash

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A
TESTBED=$(ifconfig | grep 192.168.200.211)
if [ "$TESTBED" != "" ]; then
        TESTBED="simula"
        AQMNODE="192.168.200.211"
else
        TESTBED=$(ifconfig | grep 192.168.200.2)
        if [ "$TESTBED" != "" ]; then
                TESTBED="alu"
                AQMNODE="10.187.16.1"
        else
                TESTBED="demo"
                AQMNODE="10.187.16.1"
        fi
fi
DCTCPPORT="11000"
PATH_TO_TRAFFIC_GENERATOR="dual-queue-aqm/traffic_generator"


declare -i mit_d=$1*1000
link=$2

ssh $DCTCPCLIENT 'killall http_clients_itime'
ssh $DCTCPSERVER 'killall run_httpserver'
if (( "$mit_d" != "0" )); then
        ssh $DCTCPSERVER "${PATH_TO_TRAFFIC_GENERATOR}/http_server/run_httpserver 10002 rs1.txt" &
fi
# ssh $DCTCPCLIENT 'killall http_clients_itime'
if (( "$mit_d" != "0" )); then
	ssh $DCTCPCLIENT "${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${DCTCPSERVER} 10002 rit${mit_d}_1.txt $link ${AQMNODE} ${DCTCPPORT}" &
fi

