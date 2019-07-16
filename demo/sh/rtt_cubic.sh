#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_B CLIENT=$CLIENT_B IF="$CLIENT_B_IFACE" "${HERE}/__rtt.sh"

