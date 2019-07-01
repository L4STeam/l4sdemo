#!/bin/bash

HERE=$(realpath $(dirname $0))

if [ "$#" != "1" ]; then
        echo "usage: ./cc_dctcp.sh <cc>"
        exit 65
fi

SERVER=$SERVER_A
CLIENT=$CLIENT_A

cc=$1
$HERE/set_tcp_cc.sh $SERVER $cc
$HERE/set_tcp_cc.sh $CLIENT $cc
