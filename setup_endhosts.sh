#!/bin/bash

source $(dirname $0)/environment.sh

# set up ssh keys
echo "Setting up servers and clients"
mkdir -p $HOME/.ssh
chmod 0700 $HOME/.ssh

SSH_KEY=~/.ssh/l4s-testbed
if [ ! -f $SSH_KEY ]; then
	yes '' | ssh-keygen -t rsa -f $SSH_KEY
    eval $(ssh-agent -s)
    ssh-add $SSH_KEY
fi

for machine in "$CLIENT_A" "$CLIENT_B" "$SERVER_A" "$SERVER_B"; do
	ssh -t $machine bash -xc "$(cat << EOF
whoami
mkdir -p .ssh
chmod 700 .ssh
echo "$(cat ${SSH_KEY}.pub)" >> .ssh/authorized_keys
EOF
)"
	scp -r traffic_generator $machine:.
	scp -r kernel_modules $machine:.
	scp -r common $machine:.
	scp qdisc_modules_init.sh $machine:.
done

for client in "$CLIENT_A" "$CLIENT_B"; do
	ssh -t $client bash -xc "$(cat << EOF
whoami
make -C traffic_generator/http_client
make -C traffic_generator/dl_client
chmod +x qdisc_modules_init.sh
./qdisc_modules_init.sh
EOF
)"
done

for server in "$SERVER_A" "$SERVER_B"; do
    ssh -t $server bash -xc "$(cat << EOF
whoami
make -C traffic_generator/http_server
make -C traffic_generator/dl_server
chmod +x qdisc_modules_init.sh
./qdisc_modules_init.sh
EOF
)"
done
