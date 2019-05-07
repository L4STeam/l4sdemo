#!/bin/bash

sudo tc qdisc del dev $IFACE root
cd aqms
for aqm in $(ls); do
	cd $aqm && make && sudo make unload && sudo make load
	cd ..
done
