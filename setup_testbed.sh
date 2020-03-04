#!/bin/bash

set -ex

HERE=$(realpath $(dirname $0))
source "${HERE}/environment.sh"

if which systemctl; then
    for unit in 'apt-daily.timer' 'apt-daily-upgrade.timer'; do
        sudo systemctl disable --now $unit
    done
    sudo systemctl daemon-reload
    sudo systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
fi
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
		libxrender1 libfontconfig1 libxi6 vnc4server curl

echo "Building and loading qdisc modules"
${HERE}/qdisc_modules_init.sh

${HERE}/iproute2-addons/build.sh

# install Qt 5.0.1 - has to be this version to build qwt
echo "Installing Qt - proceed with the dialog when prompted."
QT_VERSION=qt-linux-opensource-5.0.1-x86_64-offline.run
QT_INSTALLER=qt-qwt/$QT_VERSION
mkdir -p qt-qwt
if [ ! -x $QT_INSTALLER ]; then
	curl -L https://download.qt.io/archive/qt/5.0/5.0.1/$QT_VERSION -o $QT_INSTALLER
	chmod +x $QT_INSTALLER
fi
if [ ! -x /home/$(whoami)/Qt5.0.1/5.0.1/gcc_64/bin/qmake ]; then
	$QT_INSTALLER || true
fi

# install qwt
echo "Installing qwt."
QWT_DIR=/home/$(whoami)/Qt5.0.1/qwt-6.1.4/
if [ ! -d $QWT_DIR ]; then
	tar -xvf qt-qwt/qwt-6.1.4.tar.bz2 --directory=/home/$(whoami)/Qt5.0.1
	pushd $QWT_DIR
	/home/$(whoami)/Qt5.0.1/5.0.1/gcc_64/bin/qmake qwt.pro
	make -j$(nproc)
	popd
fi

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
