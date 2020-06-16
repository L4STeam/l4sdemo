pushd ../
./setup_endhosts.sh
# build and load qdisc modules
./qdisc_modules_init.sh

./iproute2-addons/build.sh
popd

# copy scripts required for experiments
for client in "$CLIENT_A" "$CLIENT_B"; do
	scp sh/*set_delay_port_netem.sh $client/.
done

# build traffic analyzer
pushd traffic_analyzer
make -j$(nproc)
popd

# build stats tools
pushd stats
make
popd


