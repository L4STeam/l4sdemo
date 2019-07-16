#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

rtt=$1

do_ssh $CLIENT "sudo tc qdisc del dev $IF root"
do_ssh $CLIENT "sudo tc qdisc add dev $IF root netem delay ${rtt}.0ms"
