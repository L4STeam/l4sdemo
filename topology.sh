# The interface facing the coordinator
IFACE_COORDINATOR=""
# Transfer rate in Mb/s
LINK_SPEED=1000

# Limits the transfer rate for the interface facing the coordinator.
ethtool -s $IFACE_COORDINATOR speed $LINK_SPEED autoneg off
