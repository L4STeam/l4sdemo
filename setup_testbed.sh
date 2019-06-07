#!/bin/bash

set -ex

source $(dirname $0)/environment.sh

for unit in 'apt-daily.timer' 'apt-daily-upgrade.timer'; do
	sudo systemctl disable --now $unit
done
sudo systemctl daemon-reload
sudo systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
sudo env DEBIAN_FRONTEND=noninteractive \
 	apt-get -y purge unattended-upgrades
sudo env DEBIAN_FRONTEND=noninteractive \
	apt-get -y update
sudo env DEBIAN_FRONTEND=noninteractive \
	 apt-get -y \
		 -o Dpkg::Options::="--force-confdef" \
                 -o Dpkg::Options::="--force-confold" \
	  install \
		mesa-common-dev libgl1-mesa-dev \
		libpcap-dev libelf-dev gcc build-essential flex \
		bison automake autotools-dev autoconf libsm-dev \
		linux-headers-generic pkg-config libmnl-dev \
		libxrender1 libfontconfig1 libxi6

echo "Building and loading qdisc modules"
# build and load qdisc modules
./qdisc_modules_init.sh

echo "Building iproute2"
rm -rf iproute2-l4s
git clone git://git.kernel.org/pub/scm/network/iproute2/iproute2.git iproute2-l4s
pushd iproute2-l4s/
git checkout v$(uname -r | awk -F '.' '{ printf "%d.%d.0", $1, $2 }')
cp ../dualpi2.patch .
git apply dualpi2.patch
make -j$(nproc)
popd

# install Qt 5.0.1 - has to be this version to build qwt
echo "Installing Qt - proceed with the dialog when prompted."
QT_VERSION=qt-linux-opensource-5.0.1-x86_64-offline.run
QT_INSTALLER=qt-qwt/$QT_VERSION
if [ ! -x $QT_INSTALLER ]; then
	curl https://download.qt.io/archive/qt/5.0/5.0.1/$QT_VERSION -o $QT_INSTALLER
	chmod +x $QT_INSTALLER
fi
if [ ! -x /home/$(whoami)/Qt5.0.1/5.0.1/gcc_64/bin/qmake ]; then
	$QT_INSTALLER
fi

# install qwt
echo "Installing qwt."
QWT_DIR=/home/$(whoami)/Qt5.0.1/qwt-6.1.4/
if [ ! -d $QWT_DIR ]; then
	tar -xvf qt-qwt/qwt-6.1.4.tar.bz2 --directory=/home/$(whoami)/Qt5.0.1
fi
pushd $QWT_DIR
/home/$(whoami)/Qt5.0.1/5.0.1/gcc_64/bin/qmake qwt.pro
make -j$(nproc)
popd

# add to library path for all users, so that L4SDemo can access it after it gets root permissions
echo "/home/$(whoami)/Qt5.0.1/qwt-6.1.4/lib" > qwt.conf
sudo mv qwt.conf /etc/ld.so.conf.d/.
sudo ldconfig

# build traffic analyzer
pushd traffic_analyzer
make -j$(nproc)
popd

# build GUI and set permissions to capture on the interface
echo "Building GUI."
pushd demo
/home/$(whoami)/Qt5.0.1/5.0.1/gcc_64/bin/qmake
make -j$(nproc)
popd

# set up ssh keys
echo "Setting up servers and clients"
mkdir -p $HOME/.ssh
chmod 0700 $HOME/.ssh
