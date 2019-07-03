#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: ./al_cubic.sh <1/0>"
 	exit 65
fi

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

SERVER=$SERVER_A
CLIENT=$CLIENT_B

on=$1

do_ssh ${CLIENT}  'sudo killall client.x86-64.pthread' 
do_ssh ${SERVER}  'sudo killall ballserver.x86-64.pthread'

if [ "$on" == "1" ]; then
    do_ssh ${CLIENT}  "export DISPLAY=\":0\";cd Downloads/ballserver;v6/client.x86-64.pthread $SERVER > /dev/null" &
    do_ssh ${SERVER}  "export DISPLAY=\":0\";cd Downloads/ballserver;v6/ballserver.x86-64.pthread 11000 10000 2 > /dev/null" &
fi
