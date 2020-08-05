#!/bin/bash


interface_options() {
	enabled="off"
	echo "changing interface options (gro, gso, tso) to $enabled"
	for o in "gro" "gso" "tso"; do
		echo "option: $o"
		sudo ethtool -K $IFACE $o $enabled
		for i in ${REV_IFACE}; do
			sudo ethtool -K $i $o $enabled
		done
		echo "setting option on the servers and clients"
		ssh -t $SERVER_A "sudo ethtool -K em2 $o $enabled"
		ssh -t $SERVER_B "sudo ethtool -K eno2 $o $enabled"
		ssh -t $CLIENT_A "sudo ethtool -K eno2 $o $enabled"
		ssh -t $CLIENT_B "sudo ethtool -K eno2 $o $enabled"
	done
	echo "changing TCP mem (mem, rmem, wmem)"
	for o in "mem='8388608 838860800 83886080000'" "rmem='4096 8738000 838860800'" "wmem='4096 65536 8388608'"; do
		echo "option: $o"
		ssh -t $SERVER_A "sudo sysctl -w net.ipv4.tcp_$o"
		ssh -t $SERVER_B "sudo sysctl -w net.ipv4.tcp_$o"
		ssh -t $CLIENT_A "sudo sysctl -w net.ipv4.tcp_$o"
		ssh -t $CLIENT_B "sudo sysctl -w net.ipv4.tcp_$o"
	done
	echo "done"
		#sleep 20
}



# do_exec <commands>
do_exec() {
	if [ "$#" == "2" ]; then
		if [ "$PRINT_COMMANDS_ONLY" == "0" ]; then
			$1 > $2
		else
			echo "$1 > $2"
		fi
	else
		if [ "$PRINT_COMMANDS_ONLY" == "0" ]; then
			$1
		else
			echo $1
		fi
	fi
}

set_aqm() {
	aqmname2=$1
	aqmpars="$2"
	declare -i rtt2=$3
	rate=$4

	if [ "$aqmname2" == "codel" ]; then
		aqmname2="codel_tst"
	fi
	if [ "$aqmname2" == "fqcodel" ]; then
		aqmname2="fq_codel"
	fi


	ctrlrate=5
	if [ $rtt2 != 0 ]; then
		rtt2=$rtt2-7
	fi

	echo "# setting $aqmname2 $aqmpars"
	do_exec "sudo ../iproute2-l4s/tc/tc qdisc del dev $IFACE root"

	do_exec "sudo ../iproute2-l4s/tc/tc qdisc add dev $IFACE root handle 1: htb default 5"
	do_exec "sudo ../iproute2-l4s/tc/tc class add dev $IFACE parent 1: classid 1:5 htb rate ${ctrlrate}Mbit"
	do_exec "sudo ../iproute2-l4s/tc/tc class add dev $IFACE parent 1: classid 1:10 htb rate ${rate}Mbit ceil ${rate}Mbit burst 1516"
	do_exec "sudo ../iproute2-l4s/tc/tc filter add dev $IFACE protocol ip parent 1:0 prio 1 u32 match ip src ${SERVER_A} flowid 1:10"
	do_exec "sudo ../iproute2-l4s/tc/tc filter add dev $IFACE protocol ip parent 1:0 prio 1 u32 match ip src ${SERVER_B} flowid 1:10"
	do_exec "sudo ../iproute2-l4s/tc/tc qdisc add dev $IFACE parent 1:10 $aqmname2 limit 40000 $aqmpars"
	
	#set extra rtt
	for i in ${REV_IFACE}; do
		do_exec "sudo tc qdisc del dev $i root"
		do_exec "sudo tc qdisc add dev $i root handle 1: prio bands 3"
		do_exec "sudo tc qdisc add dev $i parent 1:1 handle 11: netem delay 0ms limit 40000"
		do_exec "sudo tc qdisc add dev $i parent 1:2 handle 12: netem delay ${rtt2}.0ms limit 40000"
		do_exec "sudo tc filter add dev $i protocol ip parent 1:0 prio 1 u32 match ip src ${AQMNODE} classid 1:1"
		do_exec "sudo tc filter add dev $i protocol ip parent 1:0 prio 1 u32 match ip dst ${AQMNODE} classid 1:1"
		do_exec "sudo tc filter add dev $i protocol ip parent 1:0 prio 1 u32 match ip src ${SERVER_A} classid 1:2"
		do_exec "sudo tc filter add dev $i protocol ip parent 1:0 prio 1 u32 match ip dst ${SERVER_A} classid 1:2"
		do_exec "sudo tc filter add dev $i protocol ip parent 1:0 prio 1 u32 match ip src ${SERVER_B} classid 1:2"
		do_exec "sudo tc filter add dev $i protocol ip parent 1:0 prio 1 u32 match ip dst ${SERVER_B} classid 1:2"
	done

	do_exec "../iproute2-l4s/tc/tc qdisc"
}
	

set_cc(){
	ccname=$1
	aqmname="$2"
	single_run=0
	echo "# testing $ccname on $aqmname"
	cc1="${ccname:0:1}"
	cc2="${ccname:1:1}"
	if [[ $cc1 == "c" ]]; then
		cc1="cubic"
	elif [[ $cc1 == "d" ]]; then
		cc1="dctcp"
	elif [[ $cc1 == "e" ]]; then
		cc1="cubic_ecn"
	elif [[ $cc1 == "r" ]]; then
		cc1="reno"
	elif [[ $cc1 == "n" ]]; then
		cc1="reno_ecn"
	elif [[ $cc1 == "p" ]]; then
		cc1="prague"
	elif [[ $cc1 == "b" ]]; then
		cc1="bbr2"
	elif [[ $cc1 == "B" ]]; then
		cc1="bbr2_ecn"
	elif [[ $cc1 == "a" ]]; then
		cc1="prague-additive"
	elif [[ $cc1 == "s" ]]; then
		cc1="prague-scalable"
	fi

	if [[ $cc2 == "c" ]]; then
        	cc2="cubic"
    	elif [[ $cc2 == "d" ]]; then
        	cc2="dctcp"
    	elif [[ $cc2 == "e" ]]; then
        	cc2="cubic_ecn"
    	elif [[ $cc2 == "n" ]]; then
        	cc2="reno_ecn"
    	elif [[ $cc2 == "r" ]]; then
        	cc2="reno"
	elif [[ $cc2 == "p" ]]; then
		cc2="prague"
	elif [[ $cc2 == "b" ]]; then
		cc2="bbr2"
	elif [[ $cc2 == "B" ]]; then
		cc2="bbr2_ecn"
    	else
    		cc2="none"
    	fi
	
	echo "# cc1: $cc1"
	do_exec "sh/set_tcp_cc.sh $CLIENT_A $cc1"
	do_exec "sh/set_tcp_cc.sh $SERVER_A $cc1"


	if [[ $cc2 != "none" ]]; then
		echo "# cc2: $cc2"
		do_exec "sh/set_tcp_cc.sh $CLIENT_B $cc2"
		do_exec "sh/set_tcp_cc.sh $SERVER_B $cc2"
	fi
	sleep 5
}

run_test() {
	echo $1 $2 $3 $4 $5 $6 $DO_MIX $DO_RUN $DO_PLOT $PRINT_COMMANDS_ONLY

	version=1.0

	testprefix=""
	resultsfolder="res"
	
	rate=$1
	rtt=$2
	ccname=$3
	aqmname=$4
	id=$5
	aqmpars="$6"

	if [ "$PRINT_COMMANDS_ONLY" == "0" ] && [ "$DO_RUN" != "0" ]; then
		mkdir $resultsfolder
	fi

        if [ "$DO_RUN" != "0" ]; then
                set_cc $ccname $aqmname
                set_aqm $aqmname "$aqmpars" "$rtt" "$rate"
        fi


	kernel_aqm=$(uname -a)
	kernel_servera=$(ssh $SERVER_A "uname -a")
	kernel_serverb=$(ssh $SERVER_B "uname -a")
	kernel_clienta=$(ssh $CLIENT_A "uname -a")
	kernel_clientb=$(ssh $CLIENT_B "uname -a")

	cc_servera=$(ssh $SERVER_A "cat /proc/sys/net/ipv4/tcp_congestion_control")
	cc_serverb=$(ssh $SERVER_B "cat /proc/sys/net/ipv4/tcp_congestion_control")
	cc_clienta=$(ssh $CLIENT_A "cat /proc/sys/net/ipv4/tcp_congestion_control")
	cc_clientb=$(ssh $CLIENT_B "cat /proc/sys/net/ipv4/tcp_congestion_control")

	ecn_servera=$(ssh $SERVER_A "cat /proc/sys/net/ipv4/tcp_ecn")
	ecn_serverb=$(ssh $SERVER_B "cat /proc/sys/net/ipv4/tcp_ecn")
	ecn_clienta=$(ssh $CLIENT_A "cat /proc/sys/net/ipv4/tcp_ecn")
	ecn_clientb=$(ssh $CLIENT_B "cat /proc/sys/net/ipv4/tcp_ecn")

	prague_mod_servera=$(ssh $SERVER_A "modinfo tcp_prague")
	prague_mod_serverb=$(ssh $SERVER_B "modinfo tcp_prague")
	mod_dualpi2=$(modinfo sch_dualpi2)

	commit_hash=$(git rev-parse HEAD)

	info_out="
start_all v $version

$aqmname $aqmpars

id=$id
rate=$rate
rtt=$rtt
ccname=$ccname
aqmname=$aqmname
aqmpars=$aqmpars


run_test $rate $rtt \"$ccname\" \"$aqmname\" \"$id\" \"$aqmpars\"

Kernel versions:
aqmnode: $kernel_aqm
serverA: $kernel_servera
serverB: $kernel_serverb
clientA: $kernel_clienta
clientB: $kernel_clientb

TCP Congestion controls:
serverA: $cc_servera
serverB: $cc_serverb
clientA: $cc_clienta
clientB: $cc_clientb

ECN enabled:
serverA: $ecn_servera
serverB: $ecn_serverb
clientA: $ecn_clienta
clientB: $ecn_clientb

Module verions:
serverA TCP Prague: $prague_mod_servera
serverB TCP Prague: $prague_mod_serverb
DualPI2: $mod_dualpi2

commit hash: $commit_hash

"
	if [ "$DO_MIX" == "1" ]; then
		single_run="0"
		if [[ "${ccname:1:1}" == "" ]]; then
			single_run="1"
		fi
		testprefix="m_"

		testfolder="${resultsfolder}/${testprefix}${ccname}${aqmname}_"
		fid="${id}_${rate}_${rtt}"
		findex=${fid}
		lastindex=${fid}
		declare -i index=0
		while [ -d ${testfolder}${findex} ]
		do
			if [ "$index" != "0" ]; then
				lastindex="${fid}_r$index"
			fi
			index=$index+1
			findex="${fid}_r${index}"
		done

		if [ "$DO_RUN" == "0" ]; then
			echo "# processing (not running): $testfolder${lastindex}"
			datafolder=${testfolder}${lastindex}
		else

			echo "# processing: $testfolder${findex}"
			datafolder=${testfolder}${findex}
			do_exec "mkdir $datafolder"
			echo "# saving results in "$datafolder
			wait=5
			if [[ $ccname == *d* ]]; then
# wait correctly for any rate and rtt combination 
				wait="$(($wait + $rate * $rtt / 100))" 
				echo "Waiting for $wait seconds before capturing"
				if [ $rate == 200 ] && [ $rtt == 100 ]; then
					wait=250
				elif [ $rate == 120 ] && [ $rtt == 100 ]; then
					wait=150
				fi

			fi

			if [ "$PRINT_COMMANDS_ONLY" == "0" ]; then
				echo "# writing info file"
				sudo echo "$info_out" > ${datafolder}/info
				sudo ../iproute2-l4s/tc/tc qdisc >> ${datafolder}/info
			fi

			if [ "$single_run" == "0" ]; then
				cases_to_run="mix"
				if [ "$DO_EXTRA" == "1" ]; then
					cases_to_run="extra"
				elif [ "$DO_CCDF" == "1" ]; then
					cases_to_run="hl"
				fi

				
			else
				cases_to_run="single"
			fi
			
			do_exec "sh/mix.sh ${datafolder}/mix_ $rate $wait $cases_to_run"

			lastindex="${findex}"
		fi

	fi

	if [ "$DO_MIXRTT2" == "1" ]; then
		
		testprefix="mr2_"
		
		testfolder="${resultsfolder}/${testprefix}${ccname}${aqmname}_"
		fid="${id}_${rate}_${rtt}"
		findex=${fid}
		lastindex=${fid}
		declare -i index=0
		while [ -d ${testfolder}${findex} ]
		do
			if [ "$index" != "0" ]; then
				lastindex="${fid}_r$index"
			fi
			index=$index+1
			findex="${fid}_r${index}"
		done

		if [ "$DO_RUN" == "0" ]; then
			echo "# processing (not running): $testfolder${lastindex}"
			datafolder=${testfolder}${lastindex}
		else
			if [ "$DO_RUN" == "2" ] && [ "$index" != "0" ]; then
				echo "# processing (not running, already exists): $testfolder${lastindex}"
				datafolder=${testfolder}${lastindex}
			else

				echo "# processing: $testfolder${findex}"
				datafolder=${testfolder}${findex}
				do_exec "mkdir $datafolder"
				echo "# saving results in "$datafolder
				wait=5

				if [ "$PRINT_COMMANDS_ONLY" == "0" ]; then
					echo "# writing info file"
					sudo echo "$info_out" > ${datafolder}/info
					sudo ../iproute2-l4s/tc/tc qdisc >> ${datafolder}/info
				fi
				if [[ $ccname == *d* ]]; then
					# wait correctly for any rate and max rtt combination 
					wait="$(($wait + $rate * 200 / 100))" 
					echo "Waiting for $wait seconds before capturing"
				fi

				do_exec "sh/mix_rtt2.sh ${datafolder}/mix_ $rate $wait"
	
				lastindex="${findex}"
			fi
		fi

	fi


	if [ "$DO_OVERLOAD" == "1" ]; then
		single_run="0"
		if [[ "${ccname:1:1}" == "" ]]; then
			single_run="1"
		fi 
		
		testfolder="${resultsfolder}/${testprefix}${ccname}${aqmname}_"
		fid="${id}_${rate}_${rtt}"
		findex=${fid}
		lastindex=${fid}
		declare -i index=0
		while [ -d ${testfolder}${findex} ]
		do
			if [ "$index" != "0" ]; then
				lastindex="${fid}_r$index"
			fi
			index=$index+1
			findex="${fid}_r${index}"
		done

		if [ "$DO_RUN" == "0" ]; then
			echo "# processing (not running): $testfolder${lastindex}"
			datafolder=${testfolder}${lastindex}
		else
			if [ "$DO_RUN" == "2" ] && [ "$index" != "0" ]; then
				echo "# processing (not running, already exists): $testfolder${lastindex}"
				datafolder=${testfolder}${lastindex}
			else

				echo "# processing: $testfolder${findex}"
				datafolder=${testfolder}${findex}
				do_exec "mkdir $datafolder"
				if [ "$PRINT_COMMANDS_ONLY" == "0" ]; then
					echo "# writing info file"
					sudo echo "$info_out" > ${datafolder}/info
					sudo ../iproute2-l4s/tc/tc qdisc >> ${datafolder}/info
				fi
				echo "# saving results in "$datafolder
				
				if [ "$single_run" == "0" ]; then
					do_exec "sh/overload.sh ${datafolder}/mix_ $rate"
				else
					echo "TODO if needed"
				fi

				lastindex="${findex}"
			fi
		fi
	fi

}

export -f set_aqm
export -f set_cc
export -f run_test

