#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_B 
CLIENT=$CLIENT_B 
PORT="${PORT_B:-11001}" 
SAMPLE_SUFFIX=2
source "${HERE}/__wb.sh" "$@"
