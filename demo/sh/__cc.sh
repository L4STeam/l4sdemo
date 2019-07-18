#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: $0 <cc>"
 	exit 65
fi

cc=$1
HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

function set_tcp_cc()
{
    ip=$1
    tcp_cc=$2
    ecn_cc=0

    case "$tcp_cc" in
    cubic_ecn)
        tcp_cc='cubic'
        ecn_cc='1'
        ;;
    bbr_ecn)
        tcp_cc='bbr'
        ecn_cc='1'
        ;;
    prague)
        # Enable AccECN
        ecn_cc='3'
        ;;
    prague_cwr)
        ecn_cc='1'
        do_ssh $ip "sudo sysctl -w net.ipv4.tcp_force_peer_unreliable_ece=1"
        ;;
    esac

    do_ssh $ip "sudo sysctl -w net.ipv4.tcp_congestion_control=$tcp_cc"
    do_ssh $ip "sudo sysctl -w net.ipv4.tcp_ecn=$ecn_cc"
    do_ssh $ip 'sudo service ssh restart'
}

set_tcp_cc "$SERVER" "$cc"
set_tcp_cc "$CLIENT" "$cc"
