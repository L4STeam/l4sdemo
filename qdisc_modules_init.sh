#!/bin/bash

for aqm in $(ls aqms); do
	if [[ $aqm == *"dualpi2"* ]]; then
		echo "Building dualpi2 module with testbed functions enabled"
		(cd aqms/${aqm} && make IS_TESTBED=1 && sudo make unload && sudo make load)
	else
		(cd aqms/${aqm} && make && sudo make unload && sudo make load)
	fi
done
