#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A

rtt=$1
IF="$CLIENT_A_IFACE"

    do_ssh $DCTCPCLIENT "sudo tc qdisc del dev $IF root"
    do_ssh $DCTCPCLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"
