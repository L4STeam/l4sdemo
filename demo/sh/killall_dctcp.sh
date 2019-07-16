#!/bin/bash

HERE=$(realpath $(dirname $0))
env SERVER=$SERVER_A CLIENT=$CLIENT_A "${HERE}/__killall.sh"
