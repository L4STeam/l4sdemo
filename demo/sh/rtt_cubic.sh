#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

rtt=$1
IF="$CLIENT_B_IFACE"

    do_ssh $RENOCLIENT "sudo tc qdisc del dev $IF root"
    do_ssh $RENOCLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"
