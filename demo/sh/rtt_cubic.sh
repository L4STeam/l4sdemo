#!/bin/bash

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

rtt=$1
IF="$CLIENT_B_IFACE"

ssh $RENOCLIENT "sudo tc qdisc del dev $IF root"
ssh $RENOCLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"
