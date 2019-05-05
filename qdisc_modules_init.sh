#!/bin/bash

cd aqms
for aqm in $(ls aqms); do
	cd $aqm && make && sudo make load $$ cd ..
done
