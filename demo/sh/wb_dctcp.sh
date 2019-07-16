#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_A 
CLIENT=$CLIENT_A 
PORT="${PORT_A:-11000}" 
SAMPLE_SUFFIX=1
source "${HERE}/__wb.sh" "$@"
