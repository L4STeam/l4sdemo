#!/bin/bash

DCTCPSERVER=$SERVER_A
DCTCPCLIENT=$CLIENT_A
DCTCPPORT="11000"
TGEN="traffic_generator"


declare -i mit_d=$1*1000
link=$2

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh $DCTCPCLIENT 'killall http_clients_itime'
do_ssh $DCTCPSERVER 'killall run_httpserver'
if (( "$mit_d" != "0" )); then
    do_ssh $DCTCPSERVER "${TGEN}/http_server/run_httpserver 10002 ${TGEN}/gen_rsample/rs1.txt" &
fi
if (( "$mit_d" != "0" )); then
    do_ssh $DCTCPCLIENT "${TGEN}/http_client/http_clients_itime ${DCTCPSERVER} 10002 ${TGEN}/gen_ritime/rit${mit_d}_1.txt $link ${AQMNODE} ${DCTCPPORT}" &
fi

