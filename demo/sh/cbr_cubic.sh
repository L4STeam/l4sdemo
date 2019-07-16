#!/bin/bash

HERE=$(realpath $(dirname $0))
env SERVER=$SERVER_B CLIENT=$CLIENT_B "{HERE}/__cbr.sh" "$@"
