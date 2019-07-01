#!/bin/bash

if [ "$#" != "1" ]; then
        echo "usage: ./cbr_dctcp.sh <rate>"
        exit 65
fi

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A

rate=$1
ssh ${DCTCPSERVER}  'killall iperf'
if [ "$rate" != "0" ]; then
	ssh ${DCTCPSERVER} "sudo iptables -t mangle -F"
	ssh ${DCTCPSERVER} "sudo iptables -t mangle -A OUTPUT -p udp -j TOS --or-tos 10"
	ssh ${DCTCPSERVER} "iperf -c ${DCTCPCLIENT}  -u -t 500 -b ${rate}m" &
fi

