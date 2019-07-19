#!/bin/bash

source $(dirname $0)/environment.sh

sudo sysctl -qw net.ipv4.ip_forward=1
sudo ethtool -K $REV_IFACE gro off
sudo ethtool -K $REV_IFACE gso off
sudo ethtool -K $REV_IFACE tso off

REV=$(uname -r | awk -F '.' '{ printf "%d.%d", $1, $2 }')
for mod in ${HERE}/kernel_modules/${REV}/*; do
    if ! lsmod | grep $(basename $mod); then
        (cd $mod && sudo make load)
    fi
done

./demo/L4SDemo
