#!/bin/bash
if [ "$#" != "3" ]; then
        echo "usage: ./set_aqm.sh <aqm> <link> <brtt>"
	exit 65
fi

HERE=$(dirname $0)

aqm=$1
link=$2
brtt=$3
iface=$IFACE
DCTCPSERVER=$SERVER_A
RENOSERVER=$SERVER_B
DCTCPCLIENT=$CLIENT_A
RENOCLIENT=$CLIENT_B

TESTBED="demo"

echo "Testbed is $TESTBED"

do_tc() {
	local cmd="sudo $HERE/../../iproute2-l4s/tc/tc $1"
	echo $cmd
	$cmd
}

set_aqm() {
	aqmname2=$1
	aqmpars="$2"
	declare -i rtt2=$3
	rate=$4
	if [ "$aqmname2" == "fqcodel" ]; then
		aqmname2="fq_codel"
	fi
	if [ "$TESTBED" == "alu" ]; then		
		ctrlrate=5
		rtt2=$rtt2-7
		if [ $rate -gt 40 ]; then
			rate=40
		fi
	else
		ctrlrate=400
	fi

	echo "# setting $aqmname2 $aqmpars"
	do_tc "qdisc del dev $iface root"
	do_tc "qdisc add dev $iface root handle 1: htb default 5"
	do_tc "class add dev $iface parent 1: classid 1:5 htb rate ${ctrlrate}Mbit"
	do_tc "class add dev $iface parent 1: classid 1:10 htb rate ${rate}Mbit ceil ${rate}Mbit burst 1516"
	do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src ${DCTCPSERVER}/24 flowid 1:10"
#	do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src ${DCTCPSERVER} flowid 1:10"
#	do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src ${RENOSERVER} flowid 1:10"
#	if [ "$TESTBED" == "demo" ]; then
#		do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src 10.187.255.204 flowid 1:10"
#		do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src 10.187.255.208 flowid 1:10"
#		do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src 10.187.255.215 flowid 1:10"
#		do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src 10.187.255.213 flowid 1:10"
#		do_tc "filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src 10.187.255.100 flowid 1:10"
#	fi

	#set extra rtt
	if [ "$TESTBED" == "simula" ]; then
		do_tc "qdisc del dev enp2s0f2 root"
		do_tc "qdisc del dev enp2s0f3 root"

		do_tc "qdisc add dev enp2s0f2 root netem delay ${rtt2}.0ms limit 40000"
		do_tc "qdisc add dev enp2s0f3 root netem delay ${rtt2}.0ms limit 40000"
	else
		do_tc "qdisc del dev ${REV_IFACE} root"
		do_tc "qdisc add dev ${REV_IFACE} root netem delay ${rtt2}.0ms limit 40000"
		#do_tc "qdisc del dev ${REV_IFACE} root"
		#do_tc "qdisc add dev ${REV_IFACE} root handle 1: prio"
		#do_tc "class add dev ${REV_IFACE} parent 1: classid 1:5 netem delay ${rtt2}.0ms"
		#do_tc "class add dev ${REV_IFACE} parent 1: classid 1:10 netem delay 20.0ms"
		#do_tc "filter add dev ${REV_IFACE} protocol ip parent 1:0 prio 2 u32 match ip src 10.187.255.213 flowid 1:10"
		#do_tc "filter add dev ${REV_IFACE} protocol ip parent 1:0 prio 1 flowid 1:5"
	fi

	do_tc "qdisc"

	#if [[ "$aqm" == *"_ori"* ]]; then
	#	do_tc "sudo ../../iproute2-l4s/tc/tc_working qdisc add dev $iface parent 1:10 $aqmname2 $aqmpars"
	#else
		do_tc "qdisc add dev $iface parent 1:10 $aqmname2 limit 40000 $aqmpars"
	#fi

	do_tc "qdisc"
}

log(){ local x=$1 n=2 l=-1;if [ "$2" != "" ];then n=$x;x=$2;fi;while((x));do let l+=1 x/=n;done;echo $l; }

bdp=$(($link*($brtt+10)))
log_bdp=$(log $bdp)

l_slope=$((15-$log_bdp))
c_slope=$((14-$log_bdp))

a_default=5
b_default=50
k_default=2
u_default=16ms
t_default=15ms
s_default=30ms
r_default=sojourn

echo "BDP: $bdp  Log: $log_bdp"

if [[ $aqm == "dualq" ]]; then
        set_aqm dualq "l_thresh 5 offset 0 l_slope $l_slope c_slope $c_slope l_smooth 0 c_smooth 5 l_power 1 c_power 2 l_shift 50 l_speed 0" $brtt $link
elif [[ $aqm == "singleq" ]]; then
        set_aqm dualq "l_thresh 50000 offset 0 l_slope $l_slope c_slope $c_slope l_smooth 0 c_smooth 5 l_power 1 c_power 2 l_shift 0 l_speed 0" $brtt $link
elif [[ $aqm == "singleq_fixed" ]]; then
        set_aqm dualq "l_thresh 50000 offset 0 l_slope 5 c_slope 4 l_smooth 0 c_smooth 5 l_power 1 c_power 2 l_shift 0 l_speed 0" $brtt $link
elif [[ $aqm == "dualq_no_overload" ]]; then
    	set_aqm dualq "l_thresh 5 offset 0 l_slope $l_slope c_slope $c_slope l_smooth 0 c_smooth 5 l_power 1 c_power 2" $brtt $link
#	set_aqm dualq "l_thresh 1 offset 0 l_slope 5 c_slope 3 l_smooth 0 c_smooth 5 l_power 1 c_power 2 l_shift 50 l_speed 0" 7 40
elif [[ $aqm == "dualq_fixed" ]]; then
#        set_aqm dualq "l_thresh 17 offset 0 l_slope 5 c_slope 4 l_smooth 0 c_smooth 5 l_power 1 c_power 2" $brtt $link
        set_aqm dualq "l_thresh_us 1000 offset 0 l_slope 5 c_slope 4 l_smooth 0 c_smooth 5 l_power 1 c_power 2 l_shift 50" $brtt $link
elif [[ $aqm == "dualq_fixed2" ]]; then
        set_aqm dualq "l_thresh_us 1000 offset 0 l_slope 5 c_slope 4 l_smooth 0 c_smooth 5 l_power 2 c_power 4 l_shift 50" $brtt $link
elif [[ $aqm == "dualq_fixed3" ]]; then
        set_aqm dualq "l_thresh_us 1000 offset 0 l_slope 5 c_slope 4 l_smooth 0 c_smooth 5 l_power 3 c_power 6 l_shift 50" $brtt $link
elif [[ $aqm == "dualq_ol2" ]]; then
        set_aqm dualq "l_thresh 5 offset 0 l_slope 5 c_slope 4 l_smooth 0 c_smooth 5 l_power 2 c_power 4 l_shift 50 l_speed 0" $brtt $link
elif [[ $aqm == "dualq_eq" ]]; then
        set_aqm dualq "l_thresh 1 offset 0 l_slope $l_slope c_slope $c_slope l_smooth 0 c_smooth 5 l_power 2 c_power 2 l_shift 100 l_speed 5" $brtt $link
elif [[ $aqm == "red" ]]; then
        set_aqm red "limit 1600000 min 120000 max 360000 avpkt 1000 burst 220 bandwidth ${link}Mbit" $brtt $link
elif [[ $aqm == "red_ecn" ]]; then
        set_aqm red "limit 1600000 min 120000 max 360000 avpkt 1000 burst 220 ecn bandwidth ${link}Mbit" $brtt $link
elif [[ $aqm == "pie_80" ]]; then
        set_aqm pie "target 80ms ecn" $brtt $link
elif [[ $aqm == "pie_40" ]]; then
        set_aqm pie "target 40ms ecn" $brtt $link
elif [[ $aqm == "pie" ]]; then
        set_aqm pie "ecn" $brtt $link
elif [[ $aqm == "pie_necn" ]]; then
        set_aqm pie "noecn" $brtt $link
elif [[ $aqm == "pie_10" ]]; then
        set_aqm pie "target 10ms ecn" $brtt $link
elif [[ $aqm == "pie_5" ]]; then
        set_aqm pie "target 5ms ecn" $brtt $link
elif [[ $aqm == "pie_1" ]]; then
        set_aqm pie "target 1ms ecn" $brtt $link
elif [[ $aqm == "td" ]]; then
        set_aqm pi2 "noecn target 800ms alpha 1 beta 1 limit 510" $brtt $link
elif [[ $aqm == "pi2_80" ]]; then
        set_aqm pi2 "$r_default ecn target 80ms ecn alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "pi2_40" ]]; then
        set_aqm pi2 "$r_default ecn target 40ms ecn alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "pi2" ]]; then
        set_aqm pi2 "$r_default ecn target $t_default alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "pi2_dualq" ]]; then
        set_aqm pi2 "dualq" $brtt $link
elif [[ $aqm == "pi2_necn" ]]; then
        set_aqm pi2 "$r_default noecn target $t_default alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "pi2_10" ]]; then
        set_aqm pi2 "$r_default ecn target 10ms ecn alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "pi2_5" ]]; then
        set_aqm pi2 "$r_default ecn target 5ms ecn alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "pi2_1" ]]; then
        set_aqm pi2 "$r_default ecn target 1ms ecn alpha $a_default beta $b_default tupdate $u_default k $k_default" $brtt $link
elif [[ $aqm == "dpi2" ]]; then
		set_aqm dualpi2 "dc_dualq" $brtt $link
elif [[ $aqm == "dpi2_40" ]]; then
        set_aqm dualpi2 "$r_default target 40ms alpha $a_default beta $b_default tupdate $u_default k $k_default t_shift 80ms" $brtt $link
elif [[ $aqm == "dpi2_20" ]]; then
        set_aqm pi2 "$r_default dualq target 20ms alpha $a_default beta $b_default tupdate $u_default k $k_default t_shift 40ms" $brtt $link
elif [[ $aqm == "dpi2_20_ect1" ]]; then
        set_aqm pi2 "$r_default dualq target 20ms alpha $a_default beta $b_default tupdate $u_default k 1 ect1_scal t_shift 40ms" $brtt $link
elif [[ $aqm == "dpi2_15_k2" ]]; then
        set_aqm dualpi2 "target 15ms tupdate 15ms alpha 5 beta 50 dc_dualq dc_ecn k 2 t_shift 30ms l_drop 100" $brtt $link
elif [[ $aqm == "dpi2_15_k1" ]]; then
        set_aqm dualpi2 "target 15ms tupdate 15ms alpha 5 beta 50 dc_dualq k 1 t_shift 30ms l_drop 100" $brtt $link
elif [[ $aqm == "dpi2_10" ]]; then
        set_aqm pi2 "$r_default dualq target 10ms alpha $a_default beta $b_default tupdate $u_default k $k_default t_shift 20ms" $brtt $link
elif [[ $aqm == "dpi2_5" ]]; then
        set_aqm pi2 "$r_default dualq target 5ms alpha $a_default beta $b_default tupdate $u_default k $k_default t_shift 20ms" $brtt $link
elif [[ $aqm == "dpi2_1" ]]; then
        set_aqm pi2 "$r_default dualq target 1ms alpha $a_default beta $b_default tupdate $u_default k $k_default t_shift 20ms" $brtt $link
elif [[ $aqm == "dpi2_ecnth2_ec" ]]; then
        set_aqm pi2 "$r_default dualq target 20ms alpha $a_default beta $b_default tupdate $u_default k 1 ect1_scal l_thresh 2000 t_shift 40ms " $brtt $link
elif [[ $aqm == "dpi2_ecnth4_ec" ]]; then
        set_aqm pi2 "$r_default dualq target 20ms alpha $a_default beta $b_default tupdate $u_default k 1 ect1_scal l_thresh 4000 t_shift 40ms " $brtt $link
elif [[ $aqm == "dpi2_ecnth2_dc" ]]; then
        set_aqm pi2 "$r_default dualq target 20ms alpha $a_default beta $b_default tupdate $u_default k $k_default l_thresh 2000 t_shift 40ms " $brtt $link
elif [[ $aqm == "dpi2_ecnth4_dc" ]]; then
        set_aqm pi2 "$r_default dualq target 20ms alpha $a_default beta $b_default tupdate $u_default k $k_default l_thresh 4000 t_shift 40ms " $brtt $link
elif [[ $aqm == "fqcodel" ]]; then
        set_aqm fqcodel "ecn" $brtt $link
fi
