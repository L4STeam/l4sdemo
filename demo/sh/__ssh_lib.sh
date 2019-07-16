#!/bin/bash

# Use as replacement for ssh host [options] "cmd"
function do_ssh()
{
    local hname=$1
    shift 1
    ssh "$SSH_FLAGS" \
        -oControlMaster=auto \
        -oControlPath="/tmp/ssh_${hname}.sock" \
        -oControlPersist=300s \
        -oConnectTimeout=5 \
        "$hname" "$@"
}

function clean_ssh()
{
    local hname=$1
    ssh -S "/tmp/ssh_${hname}.sock" -O exit
}
