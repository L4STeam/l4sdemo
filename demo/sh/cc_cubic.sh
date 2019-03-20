#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: ./cc_cubic.sh <cc>"
 	exit 65
fi
SERVER=$SERVER_B
CLIENT=$CLIENT_B

cc=$1
sh/set_tcp_cc.sh $SERVER $cc

if [ "$cc" == "bbr" ]; then
        cc='cubic'
fi
if [ "$cc" == "bbr_ecn" ]; then
        cc='cubic_ecn'
fi

sh/set_tcp_cc.sh $CLIENT $cc

