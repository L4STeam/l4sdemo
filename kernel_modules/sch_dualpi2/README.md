# README #

### What is this repository for? ###

sch_dualpi2: DualPI2 AQM module for Linux

#### Quick summary ####
PI2 AQM is both an extension, and a simplification of the PIE (PI-Enhanced AQM). It makes quite some heuristics unnecessary, while enabling it to control scalable (L4S = low loss, low latency and scalable) congestion controls (like DCTCP, TCP-Prague, ...). Both Reno/Cubic can be used in parallel with DCTCP, giving it the same throughput.

The DualQ option can be used to classify the L4S traffic in a priority low latency queue, enabling L4S's full low latency power. The drop/mark coupling makes sure that the flows are still fair (now only congestion window fair as, due to the different Q sizes, their RTT will be different).

DualPI2 AQM is an improved and extended version of PI2 AQM.

Main differences:

  * dualq by default
  * 2 configurable overload strategies:
    * limits the drop probability to align with maximum L4S marking
    * or switches to drop for l4s queue if drop probability exceeds configured threshhold
  * uses sojourn time to estimate queue delay
  * classic drop at either enqueue or dequeue (default), depending on what is configured
  * accepts actual values of alpha and beta parameters as floats (they had to be multiplied by 16 in PIE and PI2)


### How do I get set up? ###

PI2 and DUALPI2 AQMs are implemented as modules. In order to use any of those, you need to either reinstall iproute2 package or use a local build of tc from iproute2. The files that should be added/changed and a patch are included in this repository. Currently only tested with iproute2 version 3.16.0.

Build iproute2 locally:
- download iproute2 package from https://www.kernel.org/pub/linux/utils/net/iproute2/
- apply patch or copy over the files manually

`cd iproute2-l4s && make`

Reinstall iproute2:

`cd iproute2-l4s && make install`

Build sch_<aqm> module:

`cd sch_<aqm> && make`

Register module:

`cd sch_<aqm> && make load`

Remove module:

`cd sch_<aqm> && make unload`

Add <aqm> as a qdisc with a bottleneck of 40Mbps:

`sudo tc qdisc del dev <interface> root`

`sudo tc qdisc add dev <interface> root handle 1: htb default 10`

`sudo tc class add dev <interface> parent 1: classid 1:10 htb rate 40Mbit ceil 40Mbit burst 1516`

`sudo tc qdisc add dev <interface> parent 1:10 <aqm>`

or with parameters:

`sudo tc qdisc add dev <interface> parent 1:10 dualpi2 <parameters>`

* Dependencies:

  bison libdb-dev flex


### Command line parameters for dualpi2 ###

All time parameters are in TC-defined time units and can be followed by an optional unit [s|ms|us] or used as bare numbers in microseconds (Example: 20ms or 20000).

`limit` [packets] : number of packets that can be enqueued

`target` [time] : user specified target delay in pschedtime

`tupdate` [time] : timer frequency

`alpha` : integral gain factor, used to scale the change in queuing time

`beta` : proportional gain factor, used to scale the change in queuing time comparing to previous measurement

`no_dualq | l4s_dualq | dc_dualq` :

  * `no_dualq` : single queue
  * `l4s_dualq` : dual queue for ECT(1) and CE
  * `dc_dualq` : dual queue for ECT(0), ECT(1) and CE (DCTCP compatibility)

`k` :	coupling rate factor between Classic and L4S

`no_ecn | classic_ecn | l4s_ecn | dc_ecn` :

  * `no_ecn` : no scalable marking
  * `classic_ecn` : ecn (marking) support
  * `l4s_ecn` : scalable marking for ECT(1) only
  * `dc_ecn` : scalable marking for ECT(0) and ECT(1) (DCTCP compatibility)

`l_thresh` [time] : sojourn queue size when LL packets get marked

`t_shift` [time] : L4S FIFO time shift

`c_limit | l_drop` :

  * `c_limit` : limit Classic taildrop to 100% probability divided by k, sets l_drop to 0: no drop applied at all to ECT packets
  * `l_drop` [PROBABILITY between 0 and 100] : L4S max probability where classic drop is applied to all traffic

`drop_enqueue | drop_dequeue`

  * `drop_enqueue` : drop on enqueue
  * `drop_dequeue` : drop on dequeue


### Default  parameters for dualpi2 ###

limit  : 1000

target : 20ms

tupdate	: 30ms

alpha :	0.3125

beta : 3.125

l4s_dualq

k :	2

l4s_ecn

l_thresh : 1ms

t_shift : 40ms

c_limit

drop_dequeue

### Who do I talk to? ###

* Repo owner or admins olga@albisser.org or koen.de_schepper@nokia-bell-labs.com
* Other community or team contact
