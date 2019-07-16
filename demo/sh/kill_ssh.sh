#!/bin/bash

HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

for h in $CLIENT_A $CLIENT_B $SERVER_A $SERVER_B; do
    clean_ssh $h
done
