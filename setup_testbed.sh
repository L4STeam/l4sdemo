#!/bin/bash

IFACE=""
SRC_NET=""
PCAPFILTER=""
SERVER_A=""
SERVER_B=""
CLIENT_A=""
CLIENT_B=""

if [ "$IFACE"=="" ] || [ "$SRC_NET"=="" ] || [ "$PCAPFILTER"=="" ] || [ "$PCAPFILTER"=="" ] ||
	[ "$SERVER_B"=="" ] || [ "$CLIENT_A"=="" ] || [ "$CLIENT_B"=="" ]; then
        echo "Input variables at the top of this script should be filled out to continue."
        exit 65
fi

echo "Building and loading qdisc modules"
# build and load qdisc modules
./qdisc_modules_init.sh

echo "Building iproute2"
cd iproute2-l4s && make && cd ..

# set up ssh keys
echo "Setting up servers and clients"
mkdir -p $HOME/.ssh
chmod 0700 $HOME/.ssh

ssh-keygen -t rsa

## First create .ssh directory on each machine ##
for machine in "$CLIENT_A" "$CLIENT_B" "$SERVER_A" "$SERVER_B"; do
	ssh $machine "umask 077; test -d .ssh || mkdir .ssh"
	## cat local id.rsa.pub file and pipe over ssh to append the public key in remote machine ##
	cat $HOME/.ssh/id_rsa.pub | ssh $machine "cat >> .ssh/authorized_keys"
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
	ssh $server "cd traffic_generator/http_server; make; cd ../dl_server; make; cp ../gen_rsample/rs*.txt ~/."
done

# install dependencies 
sudo apt-get install mesa-common-dev libgl1-mesa-dev libpcap-dev libelf-dev

# install Qt 5.0.1 - has to be this version to build qwt
echo "Installing Qt - proceed with the dialog when prompted."
wget https://download.qt.io/archive/qt/5.0/5.0.1/qt-linux-opensource-5.0.1-x86_64-offline.run
mv qt-linux-opensource-5.0.1-x86_64-offline.run qt-qwt/.
qt-qwt/qt-unified-linux-x64-3.1.0-online.run

# install qwt
echo "Installing qwt."
tar -xvf qt-qwt/qwt-6.1.0.tar.bz2 --directory=/home/${whoami}/Qt5.0.1
cd /home/${whoami}/Qt5.0.1/qwt-6.1.0/
/home/olga/Qt5.0.1/5.0.1/gcc_64/bin/qmake qwt.pro && make

# add to library path for all users, so that L4SDemo can access it after it gets root permissions
echo "/home/${whoami}/Qt5.0.1/qwt-6.1.4/lib" > qwt.conf
sudo mv qwt.conf /etc/ld.so.conf.d/.

# build GUI
"Building GUI."
cd demo
/home/${whoami}/Qt5.0.1/5.0.1/gcc_64/bin/qmake && make

# set permissions to capture on the interface
sh/setcap

# Start demo
./L4SDemo

