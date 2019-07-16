#!/bin/bash

# Use as replacement for ssh host [options] "cmd"
function do_ssh()
{
    local hname=$1
    shift 1
    echo "[$hname;$SSH_FLAGS] $@"
    cmd=ssh
    if [ "x$SSH_FLAGS" != "x" ]; then
        cmd="$cmd $SSH_FLAGS"
    fi
    $cmd \
        -oControlMaster=auto \
        -oControlPath="/tmp/ssh_${hname}.sock" \
        -oControlPersist=300s \
        -oConnectTimeout=5 \
        "$hname" "$@"
}

function clean_ssh()
{
    local hname=$1
    ssh -S "/tmp/ssh_${hname}.sock" -O exit $hanme || true
}
