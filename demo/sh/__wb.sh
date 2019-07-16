#!/bin/bash

TGEN="traffic_generator"

declare -i mit_r=$1*1000
link=$2

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh $SERVER 'killall run_httpserver' &> /dev/null
do_ssh $CLIENT 'killall http_clients_itime' &> /dev/null
if [[ "$mit_r" > 0 ]]; then
    echo "Starting web request between $SERVER and $CLIENT [${mit_r};${link};${PORT};${SAMPLE_SUFFIX}]"
    # Start the following programs in background
    SSH_FLAGS="-f"
    do_ssh $SERVER "${TGEN}/http_server/run_httpserver 10002 ${TGEN}/gen_rsample/rs${SAMPLE_SUFFIX}.txt"
    do_ssh $CLIENT "${TGEN}/http_client/http_clients_itime ${SERVER} 10002 ${TGEN}/gen_ritime/rit${mit_r}_${SAMPLE_SUFFIX}.txt $link ${AQMNODE} ${PORT}"
fi

