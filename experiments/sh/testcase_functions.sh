#!/bin/bash

declare -a testcases_mix=("s1d0s1d0" "s1dhs1dh")
declare -a testcases_hl=("s1dhs1dh" "s1dls1dl")
declare -a testcases_mix_extra=("s1d0s1d0" "s2d0s2d0" "s3d0s3d0" "s4d0s4d0" "s5d0s5d0" "s6d0s6d0" "s7d0s7d0" "s8d0s8d0" "s9d0s9d0" "s10d0s10d0" "s0d0s10d0" "s1d0s9d0" "s2d0s8d0" "s3d0s7d0" "s4d0s6d0" "s6d0s4d0" "s7d0s3d0" "s8d0s2d0" "s9d0s1d0" "s10d0s0d0")
declare -a testcases_single=("s1d0s0d0")

declare -i lcount

static_ports=("5005" "5010" "5020" "5050")
dyn_ports=("11005" "11010" "11020" "11050")
delays=("5" "10" "20" "50")

PATH_TO_TRAFFIC_GENERATOR="traffic_generator/"
iface=$IFACE

mit_a=10000
mit_b=10000
a_static=0
b_static=0
a_dyn=0
b_dyn=0
foldername=""

kill_processes() {
	ssh $CLIENT_B 'sudo killall http_clients_itime iperf dl_client'
	sleep 0.5
	ssh $CLIENT_A 'sudo killall http_clients_itime iperf dl_client'
	sleep 0.5

	ssh $SERVER_B 'sudo killall run_httpserver scp iperf dl_server'
	sleep 0.5
	ssh $SERVER_A 'sudo killall run_httpserver scp iperf dl_server'
	sleep 0.5

	ssh $SERVER_B 'sudo killall dl_server'
        sleep 0.5
        ssh $SERVER_A 'sudo killall dl_server'
        sleep 0.5

	killall analyzer
}

set_dyn_captions_mix() {
	if [ "$a_dyn" == "l" ]; then
		a_dyn=100
	elif [ "$a_dyn" == "h" ]; then
		a_dyn=10
	fi
	if [ "$b_dyn" == "l" ]; then
		b_dyn=100
	elif [ "$b_dyn" == "h" ]; then
		b_dyn=10
	fi
}
set_mit_mix() {
	if [ "$a_dyn" == "l" ]; then
		mit_a=100000
	elif [ "$a_dyn" == "h" ]; then
		mit_a=10000
	fi
	if [ "$b_dyn" == "l" ]; then
		mit_b=100000
	elif [ "$b_dyn" == "h" ]; then
		mit_b=10000
	fi
}

declare -i lcount
function gen_foldername_mix() {
	lcount=1
	a_static="${testcase:$lcount:1}"
	lcount=$lcount+1
	if [ "${testcase:$lcount:1}" != "d" ]; then
		echo "right" ${testcase:$lcount:1}
		a_static=$a_static"${testcase:$lcount:1}"
		lcount=$lcount+1
	fi
	lcount=$lcount+1
	a_dyn="${testcase:$lcount:1}"

	lcount=$lcount+2
	b_static="${testcase:$lcount:1}"
	lcount=$lcount+1
	if [ "${testcase:$lcount:1}" != "d" ]; then
		b_static=$b_static"${testcase:$lcount:1}"
		lcount=$lcount+1
	fi
	lcount=$lcount+1
	b_dyn="${testcase:$lcount:1}"

	testcase_id=${a_static}
	if [ "$a_dyn" != "0" ]; then
		testcase_id=${testcase_id}${a_dyn}
	fi
	testcase_id=${testcase_id}"-"${b_static}
	if [ "$b_dyn" != "0" ]; then
		testcase_id=${testcase_id}${b_dyn}
	fi


	set_dyn_captions_mix
	set_mit_mix
	foldername="/d_"d${a_dyn}s${a_static}"_r_"d${b_dyn}s${b_static}

}

