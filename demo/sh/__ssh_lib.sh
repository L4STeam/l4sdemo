#!/bin/bash

# Use as replacement for ssh host [options] "cmd"
function do_ssh()
{
    local hname=$1
    shift 1
    cmd=ssh
    if [ "x$SSH_FLAGS" != "x" ]; then
        cmd="$cmd $SSH_FLAGS"
    fi
    echo "$cmd $hname $@"
    $cmd -n \
        -oControlMaster=auto \
        -oControlPath="/tmp/ssh_${hname}.sock" \
        -oControlPersist=300s \
        -oConnectTimeout=5 \
        "$hname" "$@" \
        > >(sed "s/^/[$hname] /") \
        2> >(sed "s/^/E[${hname}] /" >&2)
}

function clean_ssh()
{
    local hname=$1
    ssh -S "/tmp/ssh_${hname}.sock" -O exit $hname || true
}
