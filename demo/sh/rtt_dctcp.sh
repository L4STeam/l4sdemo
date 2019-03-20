#!/bin/bash

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A

rtt=$1
IF="eth0"
TESTBED=$(ifconfig | grep 192.168.200.211)
if [ "$TESTBED" != "" ]; then
        TESTBED="simula"
        IF="enp3s0"
else
        TESTBED=$(ifconfig | grep 192.168.2.10)
        if [ "$TESTBED" != "" ]; then
                TESTBED="alu"
                IF="em2"
        else
                TESTBED="demo"
        fi
fi

ssh $DCTCPCLIENT "sudo tc qdisc del dev $IF root"
ssh $DCTCPCLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"

