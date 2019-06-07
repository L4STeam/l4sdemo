#!/bin/bash


## First create .ssh directory on each machine ##
for machine in "$CLIENT_A" "$CLIENT_B" "$SERVER_A" "$SERVER_B"; do
	## copy traffic generator data
	scp -r traffic_generator $machine:.
done


# compile client traffic generator
for client in "$CLIENT_A" "$CLIENT_B"; do
	ssh $client "cd traffic_generator/http_client; make; cd ../dl_client; make; cp ../gen_rsample/rit*.txt ~/."
done

# compile server traffic generator
for server in "$SERVER_A" "$SERVER_B"; do
	ssh $server "cd traffic_generator/http_server; make; cd ../dl_server; make; cp ../gen_rsample/rs*.txt ~/."
done


