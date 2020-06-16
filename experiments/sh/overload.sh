#!/bin/bash
. sh/testcase_functions.sh

if [ "$#" != "2" ]; then
 	echo "usage: ./overload.sh <main folder> <link>"
 	exit 65
fi

mainfolder=$1
declare -i link=$2

PATH_TO_TRAFFIC_GENERATOR="traffic_generator"
pcapfilter_tcp="ip and src net 10.187.255.0/24 and tcp"
pcapfilter_udp="ip and src net 10.187.255.0/24 and udp"
NRS=250

declare -i udpb
declare -i a_udpb=0
declare -i b_udpb=0
echo "OVERLOAD"
# mit for dynamic traffic - high load by default

start_udp_flows() {
	if (( "$a_udpb" != "0" )); then
		echo "starting UDP server on SERVER A"
		a_udpb=$a_udpb*$link/100
		ssh ${SERVER_A} "iperf -c ${CLIENT_A} -u -t 500 -b ${a_udpb}m" &

	fi
		
	if (( "$b_udpb" != "0" )); then
		echo "starting UDP server on SERVER B"
		b_udpb=$b_udpb*$link/100
		ssh ${SERVER_B} "iperf -c ${CLIENT_B} -u -t 500 -b ${b_udpb}m" &
	fi
}

start_static() {
	if (( "$a_static" != "0" )); then

		sleep 0.2
		ssh $SERVER_A "${PATH_TO_TRAFFIC_GENERATOR}/dl_server/dl_server 5555" &
		sleep 2
		ssh $CLIENT_A "${PATH_TO_TRAFFIC_GENERATOR}/dl_client/dl_client ${SERVER_A} 5555 ${a_static} &" &
	fi


	if (( "$b_static" != "0" )); then

		sleep 0.2
		ssh $SERVER_B "${PATH_TO_TRAFFIC_GENERATOR}/dl_server/dl_server 5555" &
        sleep 2
        ssh $CLIENT_B "${PATH_TO_TRAFFIC_GENERATOR}/dl_client/dl_client ${SERVER_B} 5555 ${b_static} &" &

	fi
}

start_ta() {
	sudo ../traffic_analyzer/analyzer $IFACE "${pcapfilter_tcp}" ${mainfolder}1000${foldername}_tcp 1000 $NRS &
	sudo ../traffic_analyzer/analyzer $IFACE "${pcapfilter_udp}" ${mainfolder}1000${foldername}_udp 1000 $NRS &
}

mkdir ${mainfolder}1000


ssh $CLIENT_A "./unset_delay_port_netem.sh"
ssh $CLIENT_B "./unset_delay_port_netem.sh"

echo "setting ecn rule for UDP in iptables"

ssh ${SERVER_A} "sudo iptables -t mangle -F"
ssh ${SERVER_A} "sudo iptables -t mangle -A OUTPUT -p udp -j TOS --or-tos 10"

sleep 1
for udpecn in "1" "0"; do
	for udpb in 50 70 100 140 200; do 
		echo "udp rate: $udpb"
		if (( "$udpecn" == "1" )); then
			a_udpb=$udpb
			b_udpb=0
		else
			a_udpb=0
			b_udpb=$udpb
		fi
		b_static=5
		a_static=5
		foldername="d_u${a_udpb}s${a_static}_r_u${b_udpb}s${b_static}"

	
		kill_processes
		
		start_static
		
		sleep 10
		start_udp_flows
		sleep 10
		start_ta


		sleep 255
	
		kill_processes
	done
done








