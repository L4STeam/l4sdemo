#!/bin/bash
if [ "$#" != "2" ]; then
        echo "usage: ./set_tcp_cc.sh <ip> <tcp_cc>"
        exit 65
fi

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

ip=$1
tcp_cc=$2
ecn_cc=0

if [ "$tcp_cc" == "cubic_ecn" ]; then
        tcp_cc='cubic'
        ecn_cc='1'
fi

if [ "$tcp_cc" == "bbr_ecn" ]; then
        tcp_cc='bbr'
        ecn_cc='1'
fi

do_ssh $ip 'sudo sysctl -w net.ipv4.tcp_congestion_control='$tcp_cc
do_ssh $ip 'sudo sysctl -w net.ipv4.tcp_ecn='$ecn_cc
do_ssh $ip 'sudo service ssh restart'


