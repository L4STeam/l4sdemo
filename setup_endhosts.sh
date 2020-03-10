#!/bin/bash

__check_machines() {
	if [ -z "$1" ]; then
		read -p "$2 doesn't exist, proceed? " -n 1 -r
		echo
		if [[ $REPLY =~ ^[Yy]$ ]]; then
			return 0
		else
			echo "Exiting setup..."
			exit 1
		fi
	fi
}

source $(dirname $0)/environment.sh

# set up ssh keys
echo "Setting up servers and clients"
mkdir -p $HOME/.ssh
chmod 0700 $HOME/.ssh

if [ ! -f $SSH_KEY ]; then
	yes '' | ssh-keygen -t rsa -f $SSH_KEY
	eval $(ssh-agent -s)
	ssh-add $SSH_KEY
fi

# check if at least one client and one server is setup
__check_machines "$CLIENT_A" "CLIENT_A"
__check_machines "$CLIENT_B" "CLIENT_B"
__check_machines "$SERVER_A" "SERVER_A"
__check_machines "$SERVER_B" "SERVER_B"

for machine in "$CLIENT_A" "$CLIENT_B" "$SERVER_A" "$SERVER_B"; do
	if [ -n "$machine" ]; then
		ssh -t $machine bash -xc "$(
			cat <<EOF
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
	fi
done

for client in "$CLIENT_A" "$CLIENT_B"; do
	if [ -n "$client" ]; then
		ssh -t $client bash -xc "$(
			cat <<EOF
whoami
make -C traffic_generator/http_client
make -C traffic_generator/dl_client
chmod +x qdisc_modules_init.sh
./qdisc_modules_init.sh
EOF
		)"
	fi
done

for server in "$SERVER_A" "$SERVER_B"; do
	if [ -n "$server" ]; then
		ssh -t $server bash -xc "$(
			cat <<EOF
whoami
make -C traffic_generator/http_server
make -C traffic_generator/dl_server
chmod +x qdisc_modules_init.sh
./qdisc_modules_init.sh
EOF
		)"
	fi
done
