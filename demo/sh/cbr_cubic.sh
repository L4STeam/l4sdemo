#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: ./cbr_cubic.sh <rate>"
 	exit 65
fi
RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

rate=$1
ssh ${RENOSERVER} 'killall iperf'
if [ "$rate" != "0" ]; then
	ssh ${RENOSERVER}  "iperf -c ${RENOCLIENT} -u -t 500 -b ${rate}m &" &
fi
