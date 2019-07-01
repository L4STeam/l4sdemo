#!/bin/bash

HERE=$(realpath $(dirname $0))

if [ "$#" != "1" ]; then
	echo "usage: ./cc_cubic.sh <cc>"
 	exit 65
fi
SERVER=$SERVER_B
CLIENT=$CLIENT_B

cc=$1
$HERE/set_tcp_cc.sh $SERVER $cc
$HERE/set_tcp_cc.sh $CLIENT $cc
