#!/bin/bash
if [ "$#" != "2" ]; then
        echo "usage: ./set_tcp_cc.sh <ip> <tcp_cc>"
        exit 65
fi

ip=$1
tcp_cc=$2
ecn_cc=2

if [ "$tcp_cc" == "cubic_ecn" ]; then
        tcp_cc='cubic'
        ecn_cc='1'
elif [ "$tcp_cc" == "bbr_ecn" ]; then
        tcp_cc='bbr'
        ecn_cc='1'
elif [ "$tcp_cc" == "bbr2_ecn" ]; then
        tcp_cc='bbr2'
        ecn_cc='1'
	ssh $ip "sudo modprobe tcp_bbr2"
	ssh $ip "sudo bash -c 'echo 1 > /sys/module/tcp_bbr2/parameters/ecn_enable'"
	ssh $ip "sudo bash -c 'echo 0 > /sys/module/tcp_bbr2/parameters/ecn_max_rtt_us'"
elif [ "$tcp_cc" == "bbr2" ]; then
	ssh $ip "sudo modprobe tcp_bbr2"
	ssh $ip "sudo bash -c 'echo 0 > /sys/module/tcp_bbr2/parameters/ecn_enable'"
elif [ "$tcp_cc" == "prague" ]; then
	ssh $ip "sudo modprobe tcp_prague"
	ssh $ip "sudo bash -c 'sudo echo 1 > /sys/module/tcp_prague/parameters/prague_rtt_scaling'"
	    ecn_cc='3'
elif [ "$tcp_cc" == "prague-additive" ]; then
	ssh $ip "sudo modprobe tcp_prague"
	ssh $ip "sudo bash -c 'sudo echo 2 > /sys/module/tcp_prague/parameters/prague_rtt_scaling'"
	ssh $ip "sudo bash -c 'sudo echo 14500 > /sys/module/tcp_prague/parameters/prague_rtt_target'"
elif [ "$tcp_cc" == "prague-scalable" ]; then
	ssh $ip "sudo modprobe tcp_prague"
	ssh $ip "sudo bash -c 'sudo echo 2 > /sys/module/tcp_prague/parameters/prague_rtt_scaling'"
	ssh $ip "sudo bash -c 'sudo echo 14500 > /sys/module/tcp_prague/parameters/prague_rtt_target'"
elif [ "$tcp_cc" == "prague_rtt" ]; then
	tcp_cc='prague'
	ssh $ip "bash -c 'cd /home/inton/tcp_prague; sudo make unload; sudo make load'"
	ssh $ip "sudo bash -c 'echo 15000 > /sys/module/tcp_prague/parameters/prague_rtt_target_usec'"
elif [ "$tcp_cc" == "reno_ecn" ]; then
        tcp_cc='reno'
        ecn_cc='1'
fi

ssh $ip "sudo sysctl -w net.ipv4.tcp_congestion_control=$tcp_cc"
ssh $ip -O exit
ssh $ip "sudo service ssh restart"
ssh $ip -O exit
ssh $ip "sudo sysctl -w net.ipv4.tcp_ecn=$ecn_cc"
ssh $ip "sudo tc qdisc del root dev em2" || true
ssh $ip "sudo tc qdisc del root dev eno2" || true
ssh $ip "sudo tc qdisc replace root dev em2 fq"
ssh $ip "sudo tc qdisc replace root dev eno2 fq"
