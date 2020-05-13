/* This file contains our logic for reporting drops to traffic analyzer
 * and is used by our patched versions of the different schedulers
 * we are using.
 *
 * It is only used for our testbed, and for a final implementation it
 * should not be included.
 */
#ifndef IS_TESTBED
#define IS_TESTBED 1
#endif

#include <net/inet_ecn.h>
#include "numbers.h"

/* This constant defines whether to include drop/queue level report and other
 * testbed related stuff we only want while developing our scheduler.
 */

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

static inline void testbed_metrics_init(struct testbed_metrics *testbed)
{
	testbed->drops_ecn = 0;
	testbed->drops_nonecn = 0;
}

static inline void __testbed_inc_drop_count(struct testbed_metrics *testbed,
					    u8 tos)
{
	if (tos & INET_ECN_MASK)
		testbed->drops_ecn++;
	else
		testbed->drops_nonecn++;
}


static inline __u8 ipv4_get_ip_bits(const struct iphdr *iph)
{
	
	uint32_t saddr = iph->saddr;
	__u8 tsmock = ntohl(saddr);
	return tsmock;
}

static inline void testbed_inc_drop_count(struct testbed_metrics *testbed,
					  struct sk_buff *skb)
{

	u32 wlen = skb_network_offset(skb);

	switch (tc_skb_protocol(skb)) {
	case htons(ETH_P_IP):
		wlen += sizeof(struct iphdr);
		if (!pskb_may_pull(skb, wlen))
			break;

		__testbed_inc_drop_count(testbed,
					 ipv4_get_ip_bits(ip_hdr(skb)));
		break;
	case htons(ETH_P_IPV6):
		break;
		// break for now, since traffic_analyzer skips IPV6 packets anyway
	/*	wlen += sizeof(struct iphdr);
		if (!pskb_may_pull(skb, wlen))
			break;

		__testbed_inc_drop_count(testbed,
					 ipv6_get_ip_bits(ipv6_hdr(skb)));
		break;
	*/
	}
}

static inline u32 testbed_write_drops(struct testbed_metrics *testbed, u8 tos)
{
	u32 drops, remainder;

	if (tos & INET_ECN_MASK) {
		drops = int2fl(testbed->drops_ecn, DROPS_M, DROPS_E,
			       &remainder);
		if (remainder > 10) {
			pr_info("High (>10) drops ecn remainder:  %u\n",
				remainder);
		}
		testbed->drops_ecn = (__force __u16)remainder;
	} else {
		drops = int2fl(testbed->drops_nonecn, DROPS_M, DROPS_E,
			       &remainder);
		if (remainder > 10) {
			pr_info("High (>10) drops nonecn remainder:  %u\n",
				remainder);
		}
		testbed->drops_nonecn = (__force __u16)remainder;
	}
	return drops;
}

static inline void testbed_add_metrics_ipv4(struct sk_buff *skb,
					    struct testbed_metrics *testbed,
					    u16 qdelay)
{
	struct iphdr *iph = ip_hdr(skb);
	u16 drops, id;
	u32 check;

	check = ntohs((__force __be16)iph->check) + ntohs(iph->id);
	if ((check + 1) >> 16)
		check = (check + 1) & 0xffff;
	drops = (__force __u16)testbed_write_drops(testbed, iph->tos);
	/* use upper 5 bits in id field to store number of drops before
	 * the current packet
	 */
	id = qdelay | (drops << 11);

	check -= id;
	check += check >> 16; /* adjust carry */

	iph->id = htons(id);
	iph->check = (__force __sum16)htons(check);
}

/* Add metrics used by traffic analyzer to packet before dispatching.
 * qdelay is the time in units of 1024 us that the packet spent in the queue.*/
static inline void testbed_add_metrics(struct sk_buff *skb,
				       struct testbed_metrics *testbed,
				       u32 qdelay_us)
{
	int wlen = skb_network_offset(skb);
	u32 qdelay_remainder;
	u16 qdelay;

	/* queue delay is converted from us (1024 ns; >> 10) to units
	 * of 32 us and encoded as float
	 */
	qdelay = (__force __u16)int2fl(qdelay_us >> 5, QDELAY_M, QDELAY_E,
				       &qdelay_remainder);
	if (qdelay_remainder > 20) {
		pr_info("High (>20) queue delay remainder:  %u\n",
			qdelay_remainder);
	}

	/* TODO: IPv6 support using flow label (and increase resolution?) */
	switch (tc_skb_protocol(skb)) {
	case htons(ETH_P_IP):
		wlen += sizeof(struct iphdr);
		if (!pskb_may_pull(skb, wlen) ||
		    skb_try_make_writable(skb, wlen))
			break;

		testbed_add_metrics_ipv4(skb, testbed, qdelay);
		break;
	default:
		break;
	}
}