#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh ${SERVER} 'killall iperf dl_client dl_server run_httpserver http_clients_itime' &>/dev/null
do_ssh ${CLIENT} 'killall iperf dl_client dl_server run_httpserver http_clients_itime' &>/dev/null
