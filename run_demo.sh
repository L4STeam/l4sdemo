#!/bin/bash

source $(dirname $0)/environment.sh

sudo sysctl -qw net.ipv4.ip_forward=1
sudo ethtool -K $REV_IFACE gro off
sudo ethtool -K $REV_IFACE gso off
sudo ethtool -K $REV_IFACE tso off

./demo/L4SDemo
