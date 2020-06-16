#!/bin/bash
. sh/testcase_functions.sh

if [ "$#" != "3" ]; then
 	echo "usage: ./mix_rtt2.sh <main folder> <link> <wait>"
 	exit 65
fi

mainfolder=$1
declare -i link=$2
wait=$3
link=${link}/4
NRS=250
PATH_TO_TRAFFIC_GENERATOR="traffic_generator"

echo "MIX RTT 2 flows in total"
declare -i i portnr

start_static() {

	echo "killing dl_server process on SERVER_A"
	ssh $SERVER_A "sudo killall dl_server"
	sleep 1
	ssh $CLIENT_A "sudo killall dl_client" 
	sleep 1
	echo "killing dl_server process on SERVER_B"
	ssh $SERVER_B "sudo killall dl_server"
	sleep 1
	ssh $CLIENT_B "sudo killall dl_client"
	sleep 1
	porta=$1
	portb=$2
	if (( "$porta" != "0" )); then
			ssh $SERVER_A "${PATH_TO_TRAFFIC_GENERATOR}/dl_server/dl_server ${porta}" &
			sleep 10
			ssh $CLIENT_A "${PATH_TO_TRAFFIC_GENERATOR}/dl_client/dl_client ${SERVER_A} ${porta} 1 &" &
	fi
	if (( "$portb" != "0" )); then
			ssh $SERVER_B "${PATH_TO_TRAFFIC_GENERATOR}/dl_server/dl_server ${portb}" &
        	sleep 10
        	ssh $CLIENT_B "${PATH_TO_TRAFFIC_GENERATOR}/dl_client/dl_client ${SERVER_B} ${portb} 1 &" &
	fi
}

start_ta() {
	adelay=$1
	bdelay=$2
	sudo ../traffic_analyzer/analyzer $IFACE "${PCAPFILTER}" ${mainfolder}1000${foldername}_d${adelay}_r${bdelay} 1000 $NRS &
}

declare -i lcount
mkdir ${mainfolder}1000
mkdir ${mainfolder}completion

ssh $CLIENT_A "./set_delay_port_netem.sh $CLIENT_A_IFACE $AQMNODE"
ssh $CLIENT_B "./set_delay_port_netem.sh $CLIENT_B_IFACE $AQMNODE"
sleep 1

portarray=("0" "5005" "5010" "5020" "5050" "5100")
for aport in "5005" "5100"; do 
	for bport in "${portarray[@]}"; do 
	
		kill_processes
		start_static $aport $bport

		sleep $wait
	
		start_ta $aport $bport
		sleep 1
		sleep 255
	
		kill_processes
	done
done

ssh $CLIENT_A "./unset_delay_port_netem.sh $CLIENT_A_IFACE"
ssh $CLIENT_B "./unset_delay_port_netem.sh $CLIENT_B_IFACE"

