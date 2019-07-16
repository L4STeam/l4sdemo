#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_A CLIENT=$CLIENT_A "${HERE}/__cc.sh" "$@"

