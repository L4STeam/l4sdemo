#!/bin/bash

SERVER=$SERVER_A
CLIENT=$CLIENT_A

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh ${SERVER} 'killall scp iperf'

