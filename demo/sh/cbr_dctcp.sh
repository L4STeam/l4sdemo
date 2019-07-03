#!/bin/bash

if [ "$#" != "1" ]; then
        echo "usage: ./cbr_dctcp.sh <rate>"
        exit 65
fi

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A

rate=$1
do_ssh ${DCTCPSERVER}  'killall iperf'
if [ "$rate" != "0" ]; then
    do_ssh ${DCTCPSERVER} "sudo iptables -t mangle -F"
    do_ssh ${DCTCPSERVER} "sudo iptables -t mangle -A OUTPUT -p udp -j TOS --or-tos 10"
    do_ssh ${DCTCPSERVER} "iperf -c ${DCTCPCLIENT}  -u -t 500 -b ${rate}m" &
fi

