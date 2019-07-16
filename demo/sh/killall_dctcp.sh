#!/bin/bash

HERE=$(realpath $(dirname $0))
SERVER=$SERVER_A 
CLIENT=$CLIENT_A 
source "${HERE}/__killall.sh"
