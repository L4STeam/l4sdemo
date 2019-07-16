#!/bin/bash

HERE=$(realpath $(dirname $0))
env SERVER=$SERVER_A CLIENT=$CLIENT_A IF="$CLIENT_A_IFACE" "${HERE}/__rtt.sh"
