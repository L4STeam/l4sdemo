#!/bin/bash
SERVER=$SERVER_B
CLIENT=$CLIENT_B

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh ${SERVER} 'killall scp iperf'
