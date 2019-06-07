#!/bin/bash

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A

rtt=$1
IF="$IFACE"
TESTBED=demo

ssh $DCTCPCLIENT "sudo tc qdisc del dev $IF root"
ssh $DCTCPCLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"

