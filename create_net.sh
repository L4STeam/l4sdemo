#!/bin/bash

if [ "x$DEBUG" != "x" ]; then
	set -x
fi

set -e

LAB_PREFIX=${LAB_PREFIX:-l4s}
LAB_ID=${LAB_ID:-171}  # ab
SSH_KEY=${SSH_KEY:-id_rsa.pub}

HERE=$(realpath $(dirname $0))

SERVER_BRIDGE=${LAB_PREFIX}sbr
CLIENT_BRIDGE=${LAB_PREFIX}cbr

declare -A PREFIXES
PREFIXES[$SERVER_BRIDGE]=192.168.$((100+LAB_ID))
PREFIXES[$CLIENT_BRIDGE]=192.168.$((200+LAB_ID))

declare -A MACTOH
declare -A IPADDR
declare -A HTOIP
declare -A MACTOIP
MAC_CNT=0
function gen_mac()
{
    local name=$1
    local ip=$2
	__mac=$(printf "52:54:00:%02x:cd:%02x" ${LAB_ID} ${MAC_CNT})
	MACTOH[$__mac]=$name
    IPADDR[$ip]=$name
    HTOIP[$name]=$ip
    MACTOIP[$__mac]=$ip
	((MAC_CNT++)) || true
}

function gen_client_address()
{
    gen_mac $1 ${PREFIXES[$CLIENT_BRIDGE]}.$2
}

function gen_server_address()
{
    gen_mac $1 ${PREFIXES[$SERVER_BRIDGE]}.$2
}
gen_client_address ${LAB_PREFIX}-client-a 217
MAC_CLIENT_A=$__mac
gen_client_address ${LAB_PREFIX}-client-b 216
MAC_CLIENT_B=$__mac
gen_server_address ${LAB_PREFIX}-server-a 115
MAC_SERVER_A=$__mac
gen_server_address ${LAB_PREFIX}-server-b 114
MAC_SERVER_B=$__mac
gen_client_address ${LAB_PREFIX}-aqm 1
MAC_AQM_CLIENT=$__mac
gen_server_address ${LAB_PREFIX}-aqm 1
MAC_AQM_SERVER=$__mac

declare -A MACS
MACS[$MAC_CLIENT_A]=$CLIENT_BRIDGE
MACS[$MAC_CLIENT_B]=$CLIENT_BRIDGE
MACS[$MAC_AQM_CLIENT]=$CLIENT_BRIDGE
MACS[$MAC_SERVER_A]=$SERVER_BRIDGE
MACS[$MAC_SERVER_B]=$SERVER_BRIDGE
MACS[$MAC_AQM_SERVER]=$SERVER_BRIDGE


DISTRIB_HDD=/data/cloudimg/bionic-server-cloudimg-amd64.qcow2
if [ ! -f $DISTRIB_HDD ]; then
	TMP_HDD=${DISTRIB_HDD%.*}.img
	curl -o $TMP_HDD \
		https://cloud-images.ubuntu.com/bionic/current/bionic-server-cloudimg-amd64.img
	qemu-img convert -O qcow2 $TMP_HDD $DISTRIB_HDD
	unlink $TMP_HDD || true
	qemu-img resize -f qcow2 $DISTRIB_HDD 10G
fi

INIT_DATA_DIR=$HERE/l4sdemo


function _net-make()
{
	local XML="$1.xml"
cat > $XML << EOF
<network>
  <name>$1</name>
  <bridge name='$2' stp='off' delay='0'/>
</network>
EOF
	sudo iptables -D FORWARD -j ACCEPT -i $SERVER_BRIDGE -o $SERVER_BRIDGE || true
	sudo iptables -D FORWARD -j ACCEPT -i $CLIENT_BRIDGE -o $SERVER_BRIDGE || true
	sudo iptables -D FORWARD -j ACCEPT -i $CLIENT_BRIDGE -o $CLIENT_BRIDGE || true
	sudo iptables -D FORWARD -j ACCEPT -i $SERVER_BRIDGE -o $CLIENT_BRIDGE || true
	sudo virsh net-destroy $1 &> /dev/null || true
	sudo virsh net-undefine $1 &> /dev/null || true
	sudo ip route del ${PREFIXES[$2]}.0/24 dev $2 || true

	sudo virsh net-define $XML
	sudo virsh net-start $1

	sudo iptables -I FORWARD -j ACCEPT -i $SERVER_BRIDGE -o $SERVER_BRIDGE
	sudo iptables -I FORWARD -j ACCEPT -i $CLIENT_BRIDGE -o $SERVER_BRIDGE
	sudo iptables -I FORWARD -j ACCEPT -i $CLIENT_BRIDGE -o $CLIENT_BRIDGE
	sudo iptables -I FORWARD -j ACCEPT -i $SERVER_BRIDGE -o $CLIENT_BRIDGE

	sudo ip route add ${PREFIXES[$2]}.0/24 dev $2
	sudo ip address add ${PREFIXES[$2]}.254 dev $2
}

function create-net()
{
	_net-make ${LAB_PREFIX}-server $SERVER_BRIDGE 
	_net-make ${LAB_PREFIX}-client $CLIENT_BRIDGE
}

function user-data-img()
{
	printf "seed-$1.iso"
}

function default-mac()
{
	printf $1 | tr 'd' 'f'
}

function create_cloud_init()
{
	local name=$1
	shift 1
	local UDATA_IMG=$(user-data-img $name)
	echo "dsmode: local" > meta-data
	unlink network-config || true
	cat >> network-config << EOF
version: 2
ethernets:
  default:
    match:
      macaddress: $(default-mac $1)
    dhcp4: true
EOF
	local cnt=0
	for mac in $@; do
		if [ "x$mac" == "x$(default-mac $1)" ]; then
			continue
		fi
		local bridge=${MACS[$mac]}
		local ipaddr=${MACTOIP[$mac]}
		cat >> network-config << EOF
  if-$cnt:
    match:
      macaddress: $mac
    addresses: [${ipaddr}/24]
EOF

		if [ "$ipaddr" != ${PREFIXES[$bridge]}.1 ]; then
				cat >> network-config << EOF
    routes: 
EOF
			for other_bridge in ${!PREFIXES[@]}; do
				if [ "x$other_bridge" == "$bridge" ]; then
					continue
				fi
				cat >> network-config << EOF
      - to: ${PREFIXES[$other_bridge]}.0/24
        via: ${PREFIXES[$bridge]}.1
EOF
			done
		fi
		((cnt++)) || true
	done

	cat > user-data << EOF
#cloud-config
password: ${LAB_PREFIX}
users:
  - ${LAB_PREFIX}:
    passwd: $(echo '${LAB_PREFIX}' |  mkpasswd -s --method=SHA-512 --rounds=4096)
    home: /home/${LAB_PREFIX}
    shell: /bin/bash
    lock_passwd: False
    name: ${LAB_PREFIX}
    gecos: ${LAB_PREFIX}
    groups: [adm, audio, cdrom, dialout, floppy, video, plugdev, dip, netdev]
    sudo:  ALL=(ALL) NOPASSWD:ALL
    ssh_authorized_keys:
      - $(cat ${SSH_KEY})
chpasswd: { expire: False }
ssh_pwauth: True
hostname: $name
runcmd:
  - /usr/bin/localectl set-keymap be
EOF

	if [ "$ipaddr" == ${PREFIXES[$bridge]}.1 ]; then
		cat >> user-data << EOF
  - /usr/bin/sed -i 's/#net.ipv4.ip_forward=1/net.ipv4.ip_forward=1/' /etc/sysctl.conf
  - sysctl -w net.ipv4.ip_forward=1
EOF
	fi

	unlink $UDATA_IMG &> /dev/null || true
	genisoimage  -output $UDATA_IMG  \
		-volid cidata -joliet -rock \
		user-data meta-data network-config
	cat user-data
	cat meta-data 
	cat network-config
}

function disk-name()
{
	printf "${1}.qcow2"
}

function create_disk()
{
	local DISK="$(disk-name $1)"
	unlink $DISK &> /dev/null || true
	qemu-img create -o backing_file=$DISTRIB_HDD,backing_fmt=qcow2 -f qcow2 $DISK
}

function _virt-install()
{
	local name=${MACTOH[$1]}
	virsh destroy $name &> /dev/null || true
	virsh -c qemu:///system undefine $name &> /dev/null || true

	create_disk $name
	create_cloud_init $name "$@"

	local net_str="$net_str --network default,mac=$(default-mac $1)"
	for mac in $@; do
		net_str="$net_str --network bridge=${MACS[$mac]},mac=$mac"
	done

	virt-install --connect=qemu:///system \
		--name "$name" \
		--ram 2048 \
		--vcpus=2 \
		--os-type=linux \
		--os-variant=ubuntu18.04 \
		--disk "$(disk-name $name),format=qcow2,device=disk,bus=virtio" \
		--disk "$(user-data-img $name),format=raw,device=cdrom" \
		--graphics none \
		$net_str \
		--noautoconsole \
		--import
}

function create_network()
{
	create-net
	_virt-install $MAC_AQM_SERVER $MAC_AQM_CLIENT
	_virt-install $MAC_CLIENT_A
	_virt-install $MAC_CLIENT_B
	_virt-install $MAC_SERVER_A
	_virt-install $MAC_SERVER_B

	echo "IP allocations:"
	for i in "${!IPADDR[@]}"; do
		echo "${i}=${IPADDR[$i]}"
	done
	echo ""
	echo "MAC allocations:"
	for i in "${!MACS[@]}"; do
		echo "${i}=${MACS[$i]}"
	done
}

function host_ip()
{
	printf "${HTOIP[${MACTOH[$1]}]}"
}

function wait-for-host()
{
	virsh start ${IPADDR[$1]} || true
	ssh-keygen -f "$HOME/.ssh/known_hosts" -R "$1" || true
	while ! yes 'yes' | ssh -o ConnectTimeout=5 \
	  -o StrictHostKeyChecking=accept-new "${LAB_PREFIX}@$1" ls; do
		sleep 1
	done
}

function ssh-addr()
{
	printf "${LAB_PREFIX}@$1"
}

function do-ssh()
{
	local ipaddr=$(ssh-addr $1)
	shift 1
	[[ $# -gt 0 ]] && exec <<< $*
	ssh -T $ipaddr
}

function provision-aqm()
{
	local ipaddr=$(host_ip $MAC_AQM_SERVER)
	wait-for-host $ipaddr
	scp -r $INIT_DATA_DIR "${LAB_PREFIX}@$ipaddr:." || true
	local demo_dir=$(basename $INIT_DATA_DIR)
	cat > environment.local << EOF
#!/bin/bash
for i in /sys/class/net/*; do
	if [ "x\$(cat \${i}/address)" == "x$MAC_AQM_CLIENT" ]; then
		export IFACE="\$(basename \$i)"
		break
	fi
done
for i in /sys/class/net/*; do
	if [ "x\$(cat \${i}/address)" == "x$MAC_AQM_SERVER" ]; then
		export REV_IFACE="\$(basename \$i)"
		break
	fi
done
export SRC_NET="${PREFIXES[$CLIENT_BRIDGE]}"
export SERVER_A=$(host_ip $MAC_SERVER_A)
export SERVER_B=$(host_ip $MAC_SERVER_B)
export CLIENT_A=$(host_ip $MAC_CLIENT_A)
export CLIENT_B=$(host_ip $MAC_CLIENT_B)
for i in /sys/class/net/*; do
	if [ "x\$(cat \${i}/address)" == "x$MAC_CLIENT_B" ]; then
		export CLIENT_B_IFACE="\$(basename \$i)"
		break
	fi
done
for i in /sys/class/net/*; do
	if [ "x\$(cat \${i}/address)" == "x$MAC_CLIENT_A" ]; then
		export CLIENT_A_IFACE="\$(basename \$i)"
		break
	fi
done
EOF
	scp environment.local "$(ssh-addr $ipaddr):./${demo_dir}/"
	do-ssh $ipaddr << EOF
chmod +x ${demo_dir}/environment.local
${demo_dir}/setup_testbed.sh
${demo_dir}/setup_endhosts.sh
$1
EOF
}

function provision-hosts()
{
	for mac in $MAC_SERVER_A $MAC_SERVER_B $MAC_CLIENT_A $MAC_CLIENT_B; do
		local ipaddr=$(host_ip $mac)
		wait-for-host $ipaddr
		do-ssh $ipaddr << EOF
sudo systemctl disable --now apt-daily.timer
sudo systemctl disable --now apt-daily-upgrade.timer
$1
sudo systemctl daemon-reload
sudo systemd-run --property='After=apt-daily.service apt-daily-upgrade.service' --wait /bin/true
sudo env DEBIAN_FRONTEND=noninteractive apt-get -y purge unattended-upgrades
sudo env DEBIAN_FRONTEND=noninteractive apt-get -y update
sudo env DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::='--force-confdef' -o Dpkg::Options::='--force-confold' install build-essential gcc
EOF
	done
}

function provision()
{
	local hosts="bash -c 'echo -e \'"
	for i in "${!IPADDR[@]}"; do
		hosts="${hosts}${i} ${IPADDR[$i]}\\n"
	done
	hosts="${hosts}\' | sudo tee -a /etc/hosts'"
	provision-hosts "$hosts"
	provision-aqm "$hosts"
}

function usage()
{
	echo "Usage: $0 [-achp]"
	echo ""
	echo "     -a   Provision the aqm"
	echo "     -c   Create the network"
	echo "     -h   This message"
	echo "     -p   Provision the network"
	echo ""
	echo "ENV VARS:"
	echo "     DEBUG      [default='$DEBUG']"
	echo "         Trace script execution if set to anything else than ''."
	echo "     LAB_PREFIX [default='$LAB_PREFIX']"
	echo "         The prefix for the lab. This will also be the username/password on the vms."
	echo "     LAB_ID     [default=$LAB_ID]"
	echo "         The lab ID (number)."
}

if [[ $# > 0 ]]; then
	while getopts "acph" o; do
		case "${o}" in
		a)
			provision-aqm
			;;
		c)
			create_network
			;;
		p)
			provision
			;;
		h)
			usage
			;;
		*)
			usage
			;;
		esac
	done
else
	usage
fi
