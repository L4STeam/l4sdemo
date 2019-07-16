#!/bin/bash

HERE=$(realpath $(dirname $0))
env SERVER=$SERVER_A CLIENT=$CLIENT_A PORT="${PORT_A:-11000}" SAMPLE_SUFFIX=1 \
    "${HERE}/__wb.sh" "$@"
