#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"


export IFACE="enp3s0"
# The interface facing the servers
export REV_IFACE="enp3s0"
# The IP prefix of the servers
export SRC_NET="192.168.60.0/24"
# Client and servers addresses
export CLIENT_A="192.168.60.235"
export CLIENT_B="192.168.60.228"
export SERVER_A="192.168.60.124"
export SERVER_B="192.168.60.223"
# The interface on both clients connected to the aqm/router (to apply mixed rtt)
export CLIENT_A_IFACE="enp2s0f1"
export CLIENT_B_IFACE="eno1"
# Server interfaces that might need to be tuned (e.g., offload, ...)
export SERVER_A_IFACE="eno1"
export SERVER_B_IFACE="eno1"

# Ports used to send web request completion times to the AQM node
export PORT_A=11000
export PORT_B=11001
# Port used by the traffic flow generators
export DL_PORT=5555
# SSH Key to use in the lab
export SSH_KEY=~/.ssh/id_rsa.pub

# The following variables should be good as-is
export PCAPFILTER="(vlan and ip and src net $SRC_NET) or (ip and src net $SRC_NET)"
export AQMNODE=$(ip -4 address show dev $IFACE | awk -F '[ /]' '/inet/ { print $6; exit 0 }')

function deactivate_offload()
{
    local hname=$1
    local ifname=$2
    for offload in tso gso gro; do
        do_ssh $hname "sudo ethtool -K $ifname $offload off"
    done
}

deactivate_offload $CLIENT_A $CLIENT_A_IFACE
deactivate_offload $CLIENT_B $CLIENT_B_IFACE
deactivate_offload $SERVER_A $SERVER_A_IFACE
deactivate_offload $SERVER_B $SERVER_B_IFACE
