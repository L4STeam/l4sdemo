#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

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
