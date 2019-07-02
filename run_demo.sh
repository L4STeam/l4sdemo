#!/bin/bash

source $(dirname $0)/environment.sh

sudo sysctl -w net.ipv4.ip_forward=1

./demo/L4SDemo
