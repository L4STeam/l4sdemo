#!/bin/bash

ip=$1

ping -c1 $ip &> /dev/null
res=$?
echo $res
exit $res
# 0=up, 1=down
