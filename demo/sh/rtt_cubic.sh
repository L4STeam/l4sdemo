#!/bin/bash

RENOSERVER=$SERVER_B
RENOCLIENT=$CLIENT_B

rtt=$1
IF="$IFACE"
TESTBED=demo

ssh $RENOCLIENT "sudo tc qdisc del dev $IF root"
ssh $RENOCLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"

