#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_B 
CLIENT=$CLIENT_B 
source "${HERE}/__killall.sh"
