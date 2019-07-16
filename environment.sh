# Source this file

# The interface facing the clients
export IFACE="ens3"
# The interface facing the servers
export REV_IFACE="ens2"
# The IP prefix of the servers
export SRC_NET="192.168.100.0/24"
# Client and servers addresses
export SERVER_A="192.168.100.115"
export SERVER_B="192.168.100.114"
export CLIENT_A="192.168.200.117"
export CLIENT_B="192.168.200.116"
# The interface on both clients connected to the aqm/router (to apply mixed rtt)
export CLIENT_A_IFACE="eth0"
export CLIENT_B_IFACE="eth0"
# Ports used to send web request completion times to the AQM node
export PORT_A=11000
export PORT_B=11001
# Port used by the traffic flow generators
export DL_PORT=5555
# SSH Key to use in the lab
export SSH_KEY=~/.ssh/l4s-testbed

# Attempt to load local settings that would override some of the above
if [ -x "environment.local" ]; then
    source environment.local
fi

eval $(ssh-agent -s)
ssh-add "${SSH_KEY}"

# The following variables should be good as-is
export PCAPFILTER="ip and src net $SRC_NET"
export AQMNODE=$(ip -4 address show dev $IFACE | awk -F '[ /]' '/inet/ { print $6; exit 0 }')

