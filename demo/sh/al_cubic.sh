#!/bin/bash

if [ "$#" != "1" ]; then
	echo "usage: ./al_cubic.sh <1/0>"
 	exit 65
fi
SERVER=$SERVER_A
CLIENT=$CLIENT_B

on=$1

ssh ${CLIENT}  'sudo killall client.x86-64.pthread' 
ssh ${SERVER}  'sudo killall ballserver.x86-64.pthread'
#sleep 1

if [ "$on" == "1" ]; then
	ssh ${CLIENT}  "export DISPLAY=\":0\";cd Downloads/ballserver;v6/client.x86-64.pthread $SERVER > /dev/null" &
	ssh ${SERVER}  "export DISPLAY=\":0\";cd Downloads/ballserver;v6/ballserver.x86-64.pthread 11000 10000 2 > /dev/null" &
fi
