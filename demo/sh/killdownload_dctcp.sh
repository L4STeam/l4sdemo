#!/bin/bash
HERE=$(realpath $(dirname $0))
source "${HERE}/__ssh_lib.sh"

do_ssh $SERVER_A "killall scp"
