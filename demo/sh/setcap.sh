#!/bin/bash
HERE=$(realpath $(dirname $0))

sudo setcap cap_net_raw,cap_net_admin=eip "${HERE}/../L4SDemo"
