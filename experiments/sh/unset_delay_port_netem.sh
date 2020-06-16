#!/bin/bash
iface=$1

sudo tc qdisc del dev $iface root

