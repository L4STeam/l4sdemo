#!/bin/bash
iface=$1
aqmnode=$2
declare -a delays=(5 10 20 50 100 200)
declare -i h band port

sudo tc qdisc del dev $iface root

sudo tc qdisc add dev $iface root handle 1: prio bands 9
sudo tc qdisc add dev $iface parent 1:1 handle 11: netem delay 0ms limit 40000

band=2
echo "setting netem delay on 8 bands:"
for d in ${delays[@]}; do
	h=10+${band}
	sudo tc qdisc add dev $iface parent 1:${band} handle ${h}: netem delay ${d}ms limit 40000
	echo "band $band handle $h delay $d"
	band=band+1
done

#first band: no extra delay for ssh commands coming from the router
sudo tc filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip src ${aqmnode} classid 1:1
sudo tc filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip dst ${aqmnode} classid 1:1

#extra delay for respective port
echo "adding filters:"
band=2
for d in "${delays[@]}"; do
	port=5000+${d}
	sudo tc filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip dport $port 0xffff classid 1:${band}
	echo "port: $port band: $band"

	port=11000+${d}
        sudo tc filter add dev $iface protocol ip parent 1:0 prio 1 u32 match ip sport $port 0xffff classid 1:${band}
        echo "port: $port band: $band"

	band=$band+1
done

tc qdisc show dev $iface
tc filter show dev $iface


