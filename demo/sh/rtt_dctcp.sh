#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_A 
CLIENT=$CLIENT_A
IF="$CLIENT_A_IFACE"
source "${HERE}/__rtt.sh"
