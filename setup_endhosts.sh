#!/bin/bash

source $(dirname $0)/environment.sh

SSH_KEY=~/.ssh/l4s-testbed
if [ ! -f $SSH_KEY ]; then
	yes '' | ssh-keygen -t rsa -f $SSH_KEY
fi

## First create .ssh directory on each machine ##
for machine in "$CLIENT_A" "$CLIENT_B" "$SERVER_A" "$SERVER_B"; do
	ssh $machine "umask 077; test -d .ssh || mkdir .ssh"
	## cat local id.rsa.pub file and pipe over ssh to append the public key in remote machine ##
	cat ${SSH_KEY}.pub | ssh $machine "cat >> .ssh/authorized_keys"
	## copy traffic generator data
	scp -r traffic_generator $machine:.
done

## This might not be necessary
eval $(ssh-agent)
ssh-add

# compile client traffic generator
for client in "$CLIENT_A" "$CLIENT_B"; do
	ssh $client "cd traffic_generator/http_client; make; cd ../dl_client; make; cp ../gen_rsample/rit*.txt ~/."
done

# compile server traffic generator
for server in "$SERVER_A" "$SERVER_B"; do
	ssh $serve "cd traffic_generator/http_server; make; cd ../dl_server; make; cp ../gen_rsample/rs*.txt ~/."
done
