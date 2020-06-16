#!/bin/bash
. sh/testcase_functions.sh

if [ "$#" != "4" ]; then
 	echo "usage: ./mix.sh <main folder> <link> <wait> <extra_cases>"
	echo "Received: $@"
 	exit 65
fi

mainfolder=$1
link=$2
declare -i wait=$3+4
testcases_to_run=$4

PATH_TO_TRAFFIC_GENERATOR="traffic_generator"
iface=$IFACE
pcapfilter=$PCAPFILTER
SAMPLE_TIME_MS=1000
NRS=250

echo "MIX"
# mit for dynamic traffic - high load by default

start_dyn_servers() {
	if (( "$a_dyn" != "0" )); then
		echo "starting SERVER A"
		echo "ssh $SERVER_A ${PATH_TO_TRAFFIC_GENERATOR}/http_server/run_httpserver 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rs1.txt"
		ssh $SERVER_A "${PATH_TO_TRAFFIC_GENERATOR}/http_server/run_httpserver 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rs1.txt" &
		sleep 2
	fi
		
	if (( "$b_dyn" != "0" )); then
		echo "starting SERVER B" 
		echo "ssh $SERVER_B ${PATH_TO_TRAFFIC_GENERATOR}/http_server/run_httpserver 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rs2.txt"
		ssh $SERVER_B "${PATH_TO_TRAFFIC_GENERATOR}/http_server/run_httpserver 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rs2.txt" &
		sleep 2
	fi
}

start_dyn_clients() {
	if (( "$a_dyn" != "0" )); then
		echo "starting dynamic clients on CLIENT A"
		echo "ssh $CLIENT_A ${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${SERVER_A} 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rit${mit_a}_1.txt ${link}"
		ssh $CLIENT_A "${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${SERVER_A} 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rit${mit_a}_1.txt ${link}" &
		sleep 0.1
	fi
		
	if (( "$b_dyn" != "0" )); then
		echo "starting dynamic clients on CLIENT B"
		echo ssh $CLIENT_B "${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${SERVER_B} 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rit${mit_b}_2.txt ${link}"
		ssh $CLIENT_B "${PATH_TO_TRAFFIC_GENERATOR}/http_client/http_clients_itime ${SERVER_B} 10002 ${PATH_TO_TRAFFIC_GENERATOR}/gen_rsample/rit${mit_b}_2.txt ${link}" &
		sleep 0.1
	fi
}

start_static() {
	ssh $SERVER_A "sudo killall dl_server" 
	ssh $CLIENT_A "sudo killall dl_client" 
	
	if (( "$a_static" != "0" )); then
		sleep 1
		ssh $SERVER_A "${PATH_TO_TRAFFIC_GENERATOR}/dl_server/dl_server 5555" &
		sleep 3
		ssh $CLIENT_A "${PATH_TO_TRAFFIC_GENERATOR}/dl_client/dl_client ${SERVER_A} 5555 ${b_static} &" &
	fi

	ssh $SERVER_B "sudo killall dl_server"
	ssh $CLIENT_B "sudo killall dl_client"
	if (( "$b_static" != "0" )); then
		sleep 1
		ssh $SERVER_B "${PATH_TO_TRAFFIC_GENERATOR}/dl_server/dl_server 5555" &
        sleep 3
        ssh $CLIENT_B "${PATH_TO_TRAFFIC_GENERATOR}/dl_client/dl_client ${SERVER_B} 5555 ${a_static} &" &

	fi
}

start_ta() {
	sudo ../traffic_analyzer/analyzer $IFACE "${pcapfilter} and tcp src port 5555" ${mainfolder}1000${foldername}_static $SAMPLE_TIME_MS $NRS &
	sudo ../traffic_analyzer/analyzer $IFACE "${pcapfilter}" ${mainfolder}1000${foldername} $SAMPLE_TIME_MS $NRS &

}

copy_completion_time() {
	mkdir ${mainfolder}completion${foldername}
	if (( "$a_dyn" != "0" )); then
		for atempt in $(seq 1 10); do
			if [ ! -f ${mainfolder}completion${foldername}/completion_time_ecn ]; then
				scp ${CLIENT_A}:completion_time_port10002 ${mainfolder}completion${foldername}/completion_time_ecn
				sleep 0.1
			else
				break
			fi
		done
		for atempt in $(seq 1 10); do
			if [ ! -f ${mainfolder}completion${foldername}/completion_time_ecn_w_hs ]; then
				scp ${CLIENT_A}:completion_time_port10002_w_hs ${mainfolder}completion${foldername}/completion_time_ecn_w_hs
				sleep 0.1
			else
				break
			fi
		done
		ssh ${CLIENT_A} "rm completion_time_*"
	fi
	if (( "$b_dyn" != "0" )); then
		for atempt in $(seq 1 10); do
			if [ ! -f ${mainfolder}completion${foldername}/completion_time_nonecn ]; then
				scp ${CLIENT_B}:completion_time_port10002 ${mainfolder}completion${foldername}/completion_time_nonecn
				sleep 0.1
			else
				break
			fi
		done
		for atempt in $(seq 1 10); do
			if [ ! -f ${mainfolder}completion${foldername}/completion_time_nonecn_w_hs ]; then
				scp ${CLIENT_B}:completion_time_port10002_w_hs ${mainfolder}completion${foldername}/completion_time_nonecn_w_hs
				sleep 0.1
			else
				break
			fi
		done
		ssh ${CLIENT_B} "rm completion_time_*"
	fi
}

mkdir ${mainfolder}1000
mkdir ${mainfolder}completion

ssh $CLIENT_A "./unset_delay_port_netem.sh"
ssh $CLIENT_B "./unset_delay_port_netem.sh"
sleep 1

declare -a testcases

if [ "$testcases_to_run" == "extra" ]; then
	echo "running extra testcases"
	testcases=("${testcases_mix_extra[@]}")
elif [ "$testcases_to_run" == "mix" ]; then
	echo "running mix"
	testcases=("${testcases_mix[@]}")
elif [ "$testcases_to_run" == "hl" ]; then
	echo "running h-l scenario for ccdf plots"
	testcases=("${testcases_hl[@]}")
elif [ "$testcases_to_run" == "single" ]; then
	echo "running single client-server scenario"
	testcases=("${testcases_single[@]}")
else
	echo "invalid testcase set: "$testcases_to_run
	exit 1
fi

for testcase in "${testcases[@]}"; do 
	echo "testcase: $testcase"
	foldername=""
	gen_foldername_mix
	kill_processes
	sleep 1
	start_static

	sleep $wait

	start_dyn_servers
	
	start_ta
	sleep 1
	start_dyn_clients

	sleep 250

	kill_processes
	sleep 1
	copy_completion_time
done








