# Source this file

# The interface facing the clients
export IFACE="ens3"
# The interface facing the servers
export REV_IFACE="ens2"
# The IP prefix of the servers
export SRC_NET="192.168.100.0/24"
# Client and servers addresses
export SERVER_A="a.server.l4s"
export SERVER_B="b.server.l4s"
export CLIENT_A="a.client.l4s"
export CLIENT_B="b.client.l4s"    
# The interface on both clients connected to the aqm/router (to apply mixed rtt)
export CLIENT_A_IFACE="eth0"
export CLIENT_B_IFACE="eth0"

# The following variables should be good as-is
export PCAPFILTER="ip and src net $SRC_NET"
export AQMNODE=$(ip -4 address show dev $IFACE | awk -F '[ /]' '/inet/ { print $6; exit 0 }')

# Attempt to load local settings that would override some of the above
if [ -x "environment.local" ]; then
    source environment.local
fi
