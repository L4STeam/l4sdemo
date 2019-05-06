/* This file contains our logic for reporting drops to traffic analyzer
 * and is used by our patched versions of the different schedulers
 * we are using.
 *
 * It is only used for our testbed, and for a final implementation it
 * should not be included.
 */

#include <net/inet_ecn.h>
#include "../common/numbers.h"

/* This constant defines whether to include drop/queue level report and other
 * testbed related stuff we only want while developing our scheduler.
 */
#define IS_TESTBED 1

struct testbed_metrics {
	/* When dropping ect0 and ect1 packets we need to treat them the same as
	 * dropping a ce packet. If the scheduler is congested, having a seperate
	 * counter for ect0/ect1 would mean we need to have packets not being
	 * marked to deliver the metric. This is unlikely to happen, and would
	 * cause falsy information showing nothing being dropped.
	 */
	u16     drops_ecn;
	u16     drops_nonecn;
};

void testbed_metrics_init(struct testbed_metrics *testbed)
{
	testbed->drops_ecn = 0;
	testbed->drops_nonecn = 0;
}

void testbed_inc_drop_count(struct sk_buff *skb, struct testbed_metrics *testbed)
{
	struct iphdr* iph;
	struct ethhdr* ethh;

	ethh = eth_hdr(skb);

	/* TODO: make IPv6 compatible (but we probably won't going to use it in our testbed?) */
	if (ntohs(ethh->h_proto) == ETH_P_IP) {
		iph = ip_hdr(skb);

		if ((iph->tos & 3))
			testbed->drops_ecn++;
		else
			testbed->drops_nonecn++;
	}
}

u32 testbed_get_drops(struct iphdr *iph, struct testbed_metrics *testbed)
{
	u32 drops;
	u32 drops_remainder;

	if ((iph->tos & 3)) {
		drops = int2fl(testbed->drops_ecn, DROPS_M, DROPS_E, &drops_remainder);
		if (drops_remainder > 10) {
			pr_info("High (>10) drops ecn remainder:  %u\n", drops_remainder);
		}
		testbed->drops_ecn = (__force __u16) drops_remainder;
	} else {
		drops = int2fl(testbed->drops_nonecn, DROPS_M, DROPS_E, &drops_remainder);
		if (drops_remainder > 10) {
			pr_info("High (>10) drops nonecn remainder:  %u\n", drops_remainder);
		}
		testbed->drops_nonecn = (__force __u16) drops_remainder;
	}
	return drops;
}

/* add metrics used by traffic analyzer to packet before dispatching */
void testbed_add_metrics(struct sk_buff *skb, struct testbed_metrics *testbed)
{
	struct iphdr *iph;
	struct ethhdr *ethh;
	u32 check;
	u16 drops;
	u16 id;
	u32 qdelay;
	u32 qdelay_remainder;

	ethh = eth_hdr(skb);
	if (ntohs(ethh->h_proto) == ETH_P_IP) {
		iph = ip_hdr(skb);
		id = ntohs(iph->id);
		check = ntohs((__force __be16)iph->check);
		check += id;
		if ((check+1) >> 16) check = (check+1) & 0xffff;

#ifndef SKIP_ENCODING
		/* queue delay is converted from ns to units of 32 us and encoded as float */
		qdelay = ((__force __u64)(ktime_get_real_ns() - ktime_to_ns(skb_get_ktime(skb)))) >> 15;
		qdelay = int2fl(qdelay, QDELAY_M, QDELAY_E, &qdelay_remainder);
		if (qdelay_remainder > 20) {
			pr_info("High (>20) queue delay remainder:  %u\n", qdelay_remainder);
		}
#else /* old behaviour - no encoding */
		/* convert to ms */
		qdelay = ((__force __u64)(ktime_get_real_ns() - ktime_to_ns(skb_get_ktime(skb)))) >> 20;
		if (qdelay > 2047) {
			pr_info("Large queue delay:  %u\n", qdelay);
			qdelay = 2047;
		}
#endif

		id = (__force __u16) qdelay;
		drops = (__force __u16) testbed_get_drops(iph, testbed);
		id = id | (drops << 11); /* use upper 5 bits in id field to store number of drops before the current packet */

		check -= id;
		check += check >> 16; /* adjust carry */
		iph->id = htons(id);
		iph->check = (__force __sum16)htons(check);
	}
}
