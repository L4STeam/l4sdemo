// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018 Nokia.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Koen De Schepper <koen.de_schepper@nokia-bell-labs.com>
 * Author: Olga Albisser <olga@albisser.org>
 * Author: Henrik Steen <henrist@henrist.net>
 * Author: Olivier Tilmans <olivier.tilmans@nokia-bell-labs.com>
 *
 * DualPI Improved with a Square (dualpi2)
 * Supports controlling scalable congestion controls (DCTCP, etc...)
 * Supports DualQ with PI2
 * Supports L4S ECN identifier
 *
 * References:
 * IETF draft submission:
 *   http://tools.ietf.org/html/draft-ietf-tsvwg-aqm-dualq-coupled-08
 * ACM CoNEXT’16, Conference on emerging Networking EXperiments
 * and Technologies :
 * "PI2: PI Improved with a Square to support Scalable Congestion Controllers"
 * IETF draft submission:
 *   http://tools.ietf.org/html/draft-pan-aqm-pie-00
 * IEEE  Conference on High Performance Switching and Routing 2013 :
 * "PIE: A * Lightweight Control Scheme to Address the Bufferbloat Problem"
 * Partially based on the PIE implementation:
 * Copyright (C) 2013 Cisco Systems, Inc, 2013.
 * Author: Vijay Subramanian <vijaynsu@cisco.com>
 * Author: Mythili Prabhu <mysuryan@cisco.com>
 * ECN support is added by Naeem Khademi <naeemk@ifi.uio.no>
 * University of Oslo, Norway.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include <net/dsfield.h>
#include <linux/string.h>
#include "compat-linux.h"

#define MAX_PROB  0xffffffff
#define ALPHA_BETA_MAX 10240000

/* parameters used */
struct dualpi2_params {
	u64	target;		/* user specified target delay in nanoseconds */
	u32	tupdate;	/* timer frequency (in jiffies) */
	u32	limit;		/* number of packets that can be enqueued */
	u32	alpha;		/* alpha and beta are user specified values
				 * scaled by factor of 256
				 */
	u32	beta;		/* and are used for shift relative to 1 */
	u32	k;		/* coupling rate between Classic and L4S */
	u32	queue_mask:2,	/* Mask on ecn bits to determine if packet goes
				 * in l-queue
				 * 0 (00): single queue
				 * 1 (01): dual queue for ECT(1) and CE
				 * 3 (11): dual queue for ECT(0), ECT(1) and CE
				 *	  (DCTCP compatibility)
				 */
		mark_mask:2,	/* Mask on ecn bits to determine marking
				 * (instead of dropping)
				 * 0 (00): no ecn
				 * 3 (11): ecn (marking) support
				 */
		scal_mask:2,	/* Mask on ecn bits to mark p (instead of p^2)
				 * 0 (00): no scalable marking
				 * 1 (01): scalable marking for ECT(1)
				 * 3 (11): scalable marking for ECT(0) and
				 *	  ECT(1) (DCTCP compatibility)
				 */
		et_packets:1,   /* ecn threshold in packets (1) or us (0) */
		drop_early:1;	/* Drop at enqueue */
	u32	ecn_thresh;	/* sojourn queue size to mark LL packets */
	u32	l_drop;		/* L4S max probability where classic drop is
				 * applied to all traffic, if 0 then no drop
				 * applied at all (but taildrop) to ECT
				 * packets
				 */
	u32	dequeue_ratio;	/* WRR denominator for dequeues */
};

/* variables used */
struct dualpi2_vars {
	u64	qdelay_c;		/* Classic Q delay */
	u64	qdelay_l;		/* L4S Q delay */
	u32	prob;			/* probability scaled as u32 */
	u32	alpha;			/* calculated alpha value */
	u32	beta;			/* calculated beta value */
	u32	deferred_drop_count;
	u32	deferred_drop_len;
	u32	dequeued_l;		/* Successive L4S dequeues */
};

/* statistics gathering */
struct dualpi2_stats {
	u32	packets_in;	/* total number of packets enqueued */
	u32	dropped;	/* packets dropped due to dualpi2_action */
	u32	overlimit;	/* dropped due to lack of space in queue */
	u32	maxq;		/* maximum queue size */
	u32	ecn_mark;	/* packets marked with ECN */
};

/* private data for the Qdisc */
struct dualpi2_sched_data {
	struct Qdisc *l_queue;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
	struct Qdisc *sch;
#endif
	struct dualpi2_params params;
	struct dualpi2_vars vars;
	struct dualpi2_stats stats;
	struct timer_list adapt_timer;
	struct testbed_metrics testbed;
};

static inline void set_ts_cb(struct sk_buff *skb)
{
        u64 now = ktime_get_ns();
        memcpy(qdisc_skb_cb(skb)->data, &now, sizeof(u64));
}

static inline u64 get_ts_cb(struct sk_buff *skb)
{
        u64 ts = 0;
        memcpy(&ts, qdisc_skb_cb(skb)->data, sizeof(u64));
	return ts;
}

static inline u64 skb_sojourn_time(struct sk_buff *skb, u64 reference)
{
        return skb ? reference - get_ts_cb(skb) : 0;
}

static inline u32 __dualpi2_vars_from_params(u32 param)
{
	return (param * (MAX_PROB / NSEC_PER_SEC)) >> 8;
}

static void dualpi2_calculate_alpha_beta(struct dualpi2_sched_data *q)
{
	/* input alpha and beta should be in multiples of 1/256 */
	q->vars.alpha = __dualpi2_vars_from_params(q->params.alpha);
	q->vars.beta = __dualpi2_vars_from_params(q->params.beta);
}

static void dualpi2_params_init(struct dualpi2_params *params)
{
	params->alpha = 80;
	params->beta = 800;
	params->tupdate = usecs_to_jiffies(32 * USEC_PER_MSEC);	/* 32 ms */
	params->limit = 10000;
	params->target = 20 * NSEC_PER_MSEC;	/* 20 ms */
	params->k = 2;
	params->queue_mask = INET_ECN_ECT_1;
	params->mark_mask = INET_ECN_MASK;
	params->scal_mask = INET_ECN_ECT_1;
	params->et_packets = 0;
	params->ecn_thresh = 1000;
	params->l_drop = 0;
	params->drop_early = false;
	params->dequeue_ratio = 16;
}

static u32 get_ecn_field(struct sk_buff *skb)
{
	switch (tc_skb_protocol(skb)) {
	case htons(ETH_P_IP):
		return ipv4_get_dsfield(ip_hdr(skb)) & INET_ECN_MASK;
	case htons(ETH_P_IPV6):
		return ipv6_get_dsfield(ipv6_hdr(skb)) & INET_ECN_MASK;
	default:
		return 0;
	}
}

static bool should_drop(struct Qdisc *sch, struct dualpi2_sched_data *q,
			u32 ecn, struct sk_buff *skb)
{
	u32 mtu = psched_mtu(qdisc_dev(sch));
	u64 local_l_prob;
	bool overload;
	u32 rnd;

	/* If we have fewer than 2 mtu-sized packets, disable drop,
	 * similar to min_th in RED
	 */
	if (sch->qstats.backlog < 2 * mtu)
		return false;

	local_l_prob = (u64)q->vars.prob * q->params.k;
	overload = q->params.l_drop && local_l_prob > (u64)q->params.l_drop;

	rnd = prandom_u32();
	if (!overload && (ecn & q->params.scal_mask)) {
		/* do scalable marking */
		if (rnd < local_l_prob && INET_ECN_set_ce(skb))
			/* mark ecn without a square */
			q->stats.ecn_mark++;
	} else if (rnd < q->vars.prob) {
		/* think twice to drop, so roll again */
		rnd = prandom_u32();
		if (rnd < q->vars.prob) {
			if (!overload &&
			    (ecn & q->params.mark_mask) &&
			    INET_ECN_set_ce(skb))
				/* mark ecn with a square */
				q->stats.ecn_mark++;
			else
				return true;
		}
	}

	return false;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
static int dualpi2_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch)
#else
static int dualpi2_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch,
				 struct sk_buff **to_free)
#endif
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);
	u32 ecn = get_ecn_field(skb);
	int err;

	if (unlikely(qdisc_qlen(sch) >= sch->limit)) {
		qdisc_qstats_overlimit(sch);
		err = NET_XMIT_DROP;
		goto drop;
	}

	/* drop early if configured */
	if (q->params.drop_early && should_drop(sch, q, ecn, skb)) {
		err = NET_XMIT_SUCCESS | __NET_XMIT_BYPASS;
		goto drop;
	}

	/* set to the time the HTQ packet is in the Q */
        set_ts_cb(skb);

	q->stats.packets_in++;
	if (qdisc_qlen(sch) > q->stats.maxq)
		q->stats.maxq = qdisc_qlen(sch);

	/* decide L4S queue or classic */
	if (ecn & q->params.queue_mask) {
		sch->q.qlen++; /* otherwise packets are not seen by parent Q */
		qdisc_qstats_backlog_inc(sch, skb);
		return qdisc_enqueue_tail(skb, q->l_queue);
	} else {
		return qdisc_enqueue_tail(skb, sch);
	}

drop:
	q->stats.dropped++;
	testbed_inc_drop_count(&q->testbed, skb);
	q->vars.deferred_drop_count += 1;
        q->vars.deferred_drop_len += qdisc_pkt_len(skb);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
	qdisc_drop(skb, sch);
#else
	qdisc_drop(skb, sch, to_free);
#endif
	return err;
}

static const struct nla_policy dualpi2_policy[TCA_DUALPI2_MAX + 1] = {
	[TCA_DUALPI2_ALPHA] = {.type = NLA_U32},
	[TCA_DUALPI2_BETA] = {.type = NLA_U32},
	[TCA_DUALPI2_DUALQ] = {.type = NLA_U32},
	[TCA_DUALPI2_ECN] = {.type = NLA_U32},
	[TCA_DUALPI2_K] = {.type = NLA_U32},
	[TCA_DUALPI2_L_DROP] = {.type = NLA_U32},
	[TCA_DUALPI2_ET_PACKETS] = {.type = NLA_U32},
	[TCA_DUALPI2_L_THRESH] = {.type = NLA_U32},
	[TCA_DUALPI2_LIMIT] = {.type = NLA_U32},
	[TCA_DUALPI2_TARGET] = {.type = NLA_U32},
	[TCA_DUALPI2_TUPDATE] = {.type = NLA_U32},
	[TCA_DUALPI2_DROP_EARLY] = {.type = NLA_U32},
	[TCA_DUALPI2_WRR_RATIO] = {.type = NLA_U32},
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
static int dualpi2_change(struct Qdisc *sch, struct nlattr *opt)
#else
static int dualpi2_change(struct Qdisc *sch, struct nlattr *opt,
			  struct netlink_ext_ack *extack)
#endif
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_DUALPI2_MAX + 1];
	unsigned int qlen, dropped = 0;
	int err;

	if (!opt)
		return -EINVAL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
	err = nla_parse_nested(tb, TCA_DUALPI2_MAX, opt, dualpi2_policy);
#else
	err = nla_parse_nested(tb, TCA_DUALPI2_MAX, opt, dualpi2_policy, NULL);
#endif
	if (err < 0)
		return err;

	sch_tree_lock(sch);
	if (q->l_queue == &noop_qdisc) {
		struct Qdisc *child;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
		child = qdisc_create_dflt(sch->dev_queue, &pfifo_qdisc_ops,
					  TC_H_MAKE(sch->handle, 1));
#else
		child = qdisc_create_dflt(sch->dev_queue, &pfifo_qdisc_ops,
					  TC_H_MAKE(sch->handle, 1), extack);
#endif
		if (child)
			q->l_queue = child;
	}

	if (tb[TCA_DUALPI2_TARGET]) {
		u64 target = nla_get_u64(tb[TCA_DUALPI2_TARGET]);

		q->params.target = target * NSEC_PER_USEC;
	}

	if (tb[TCA_DUALPI2_TUPDATE]) {
		u32 tupdate_usecs = nla_get_u32(tb[TCA_DUALPI2_TUPDATE]);

		q->params.tupdate = usecs_to_jiffies(tupdate_usecs);
	}

	if (tb[TCA_DUALPI2_LIMIT]) {
		u32 limit = nla_get_u32(tb[TCA_DUALPI2_LIMIT]);

		q->params.limit = limit;
		sch->limit = limit;
	}

	if (tb[TCA_DUALPI2_ALPHA]) {
		q->params.alpha = nla_get_u32(tb[TCA_DUALPI2_ALPHA]);
		if (q->params.alpha > ALPHA_BETA_MAX)
			q->params.alpha = ALPHA_BETA_MAX;
	}

	if (tb[TCA_DUALPI2_BETA]) {
		q->params.beta = nla_get_u32(tb[TCA_DUALPI2_BETA]);
		if (q->params.beta > ALPHA_BETA_MAX)
                        q->params.beta = ALPHA_BETA_MAX;
	}

	if (tb[TCA_DUALPI2_DUALQ])
		q->params.queue_mask = nla_get_u32(tb[TCA_DUALPI2_DUALQ]);

	if (tb[TCA_DUALPI2_ECN]) {
		u32 masks = nla_get_u32(tb[TCA_DUALPI2_ECN]);

		q->params.mark_mask = (masks >> 2) & INET_ECN_MASK;
		q->params.scal_mask = masks & INET_ECN_MASK;
	}

	if (tb[TCA_DUALPI2_K])
		q->params.k = nla_get_u32(tb[TCA_DUALPI2_K]);

	if (tb[TCA_DUALPI2_ET_PACKETS])
                q->params.et_packets = nla_get_u32(tb[TCA_DUALPI2_ET_PACKETS]);

	if (tb[TCA_DUALPI2_L_THRESH])
		/* l_thresh is in us */
		q->params.ecn_thresh = nla_get_u32(tb[TCA_DUALPI2_L_THRESH]);

	if (tb[TCA_DUALPI2_L_DROP]) {
		u32 l_drop_percent = nla_get_u32(tb[TCA_DUALPI2_L_DROP]);

		q->params.l_drop = l_drop_percent * (MAX_PROB / 100);
	}

	if (tb[TCA_DUALPI2_DROP_EARLY])
		q->params.drop_early = nla_get_u32(tb[TCA_DUALPI2_DROP_EARLY]);

	if (tb[TCA_DUALPI2_WRR_RATIO])
		q->params.dequeue_ratio =
			nla_get_u32(tb[TCA_DUALPI2_WRR_RATIO]);

	/* Calculate new internal alpha and beta values in case their
	 * dependencies are changed
	 */
	dualpi2_calculate_alpha_beta(q);

	/* Drop excess packets if new limit is lower */
	qlen = sch->q.qlen;
	while (sch->q.qlen > sch->limit) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
		struct sk_buff *skb = __skb_dequeue(&sch->q);
#else
		struct sk_buff *skb = __qdisc_dequeue_head(&sch->q);

#endif

		dropped += qdisc_pkt_len(skb);
		qdisc_qstats_backlog_dec(sch, skb);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
		qdisc_drop(skb, sch);
#else
		rtnl_qdisc_drop(skb, sch);
#endif
	}
	qdisc_tree_reduce_backlog(sch, qlen - sch->q.qlen, dropped);

	sch_tree_unlock(sch);
	return 0;
}

static inline u64 qdelay_in_ns(struct Qdisc *q, u64 now)
{
	struct sk_buff *skb = qdisc_peek_head(q);
	return skb_sojourn_time(skb, now);
}

static void calculate_probability(struct Qdisc *sch)
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);
	u64 now = ktime_get_ns();
	u64 qdelay_old;
	u64 qdelay;
	u32 oldprob;
	s64 delta;	/* determines the change in probability */

	qdelay_old = max(q->vars.qdelay_c, q->vars.qdelay_l);

	q->vars.qdelay_l = qdelay_in_ns(q->l_queue, now);
	q->vars.qdelay_c = qdelay_in_ns(sch, now);

	qdelay = max(q->vars.qdelay_c, q->vars.qdelay_l);

	delta = (s64)((qdelay - q->params.target)) * q->vars.alpha;
	delta += (s64)((qdelay - qdelay_old)) * q->vars.beta;

	oldprob = q->vars.prob;

	q->vars.prob += delta;

	if (delta > 0) {
		/* prevent overflow */
		if (q->vars.prob < oldprob)
			q->vars.prob = MAX_PROB;
	} else {
		/* prevent underflow */
		if (q->vars.prob > oldprob)
			q->vars.prob = 0;
	}

	/* If no switchover to drop configured, align maximum drop probability
	 * with 100% L4S marking.
	 */
	if (!q->params.l_drop && (q->vars.prob > MAX_PROB / q->params.k))
		q->vars.prob = MAX_PROB / q->params.k;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
static void dualpi2_timer(unsigned long arg)
{
	struct Qdisc *sch = (struct Qdisc *)arg;
	struct dualpi2_sched_data *q = qdisc_priv(sch);
#else
static void dualpi2_timer(struct timer_list *timer)
{
        struct dualpi2_sched_data *q = from_timer(q, timer, adapt_timer);
	struct Qdisc *sch = q->sch;
#endif
	spinlock_t *root_lock; /* spinlock for qdisc parameter update */

	root_lock = qdisc_lock(qdisc_root_sleeping(sch));

	spin_lock(root_lock);
	calculate_probability(sch);

	/* reset the timer to fire after 'tupdate'. tupdate is in jiffies. */
	if (q->params.tupdate)
		mod_timer(&q->adapt_timer, jiffies + q->params.tupdate);
	spin_unlock(root_lock);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
static int dualpi2_init(struct Qdisc *sch, struct nlattr *opt)
#else
static int dualpi2_init(struct Qdisc *sch, struct nlattr *opt,
			struct netlink_ext_ack *extack)
#endif
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);

	dualpi2_params_init(&q->params);
	dualpi2_calculate_alpha_beta(q);
	sch->limit = q->params.limit;
	q->l_queue = &noop_qdisc;
	testbed_metrics_init(&q->testbed);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
	setup_timer(&q->adapt_timer, dualpi2_timer, (unsigned long)sch);
#else
	q->sch = sch;
        timer_setup(&q->adapt_timer, dualpi2_timer, 0);
#endif
	if (opt) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
		int err = dualpi2_change(sch, opt);
#else
		int err = dualpi2_change(sch, opt, extack);
#endif

		if (err)
			return err;
	}

	mod_timer(&q->adapt_timer, jiffies + HZ / 2);
	return 0;
}

static int dualpi2_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct nlattr *opts = nla_nest_start(skb, TCA_OPTIONS);
	struct dualpi2_sched_data *q = qdisc_priv(sch);

	u64 target_usec = q->params.target;

        do_div(target_usec, NSEC_PER_USEC);

	if (!opts)
		goto nla_put_failure;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
	if (nla_put_u64(skb, TCA_DUALPI2_TARGET,
                        target_usec) ||
#else
	if (nla_put_u64_64bit(skb, TCA_DUALPI2_TARGET,
			target_usec, TCA_DUALPI2_PAD) ||
#endif
	    nla_put_u32(skb, TCA_DUALPI2_LIMIT, sch->limit) ||
	    nla_put_u32(skb, TCA_DUALPI2_TUPDATE,
			jiffies_to_usecs(q->params.tupdate)) ||
	    nla_put_u32(skb, TCA_DUALPI2_ALPHA, q->params.alpha) ||
	    nla_put_u32(skb, TCA_DUALPI2_BETA, q->params.beta) ||
	    nla_put_u32(skb, TCA_DUALPI2_DUALQ, q->params.queue_mask) ||
	    nla_put_u32(skb, TCA_DUALPI2_ECN,
			(q->params.mark_mask << 2) | q->params.scal_mask) ||
	    nla_put_u32(skb, TCA_DUALPI2_K, q->params.k) ||
	    nla_put_u32(skb, TCA_DUALPI2_ET_PACKETS, q->params.et_packets) ||
	    nla_put_u32(skb, TCA_DUALPI2_L_THRESH, q->params.ecn_thresh) ||
	    /* put before L_DROP because we are inside a multiline expression */
	    nla_put_u32(skb, TCA_DUALPI2_DROP_EARLY, q->params.drop_early) ||
	    nla_put_u32(skb, TCA_DUALPI2_WRR_RATIO, q->params.dequeue_ratio) ||
	    nla_put_u32(skb, TCA_DUALPI2_L_DROP,
			q->params.l_drop / (MAX_PROB / 100)))
		goto nla_put_failure;

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -1;
}

static int dualpi2_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);
	u64 qdelay_c_usec = q->vars.qdelay_c;
        u64 qdelay_l_usec = q->vars.qdelay_l;

        do_div(qdelay_c_usec, NSEC_PER_USEC);
        do_div(qdelay_l_usec, NSEC_PER_USEC);

	struct tc_dualpi2_xstats st = {
		.prob		= q->vars.prob,
                .delay_c        = qdelay_c_usec,
                .delay_l        = qdelay_l_usec,
		.packets_in	= q->stats.packets_in,
		.overlimit	= q->stats.overlimit,
		.maxq		= q->stats.maxq,
		.dropped	= q->stats.dropped,
		.ecn_mark	= q->stats.ecn_mark,
	};

	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static struct sk_buff *dualpi2_qdisc_dequeue(struct Qdisc *sch)
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);
	struct sk_buff *skb_l, *skb_c, *skb;
	u64 qdelay_l, qdelay_c;
	u32 qlen_l;
	u64 now;

pick_packet:
	skb_l = qdisc_peek_head(q->l_queue);
	skb_c = qdisc_peek_head(sch);
	skb = NULL;
	now = ktime_get_ns();
	qdelay_l = skb_sojourn_time(skb_l, now);
	qdelay_c = skb_sojourn_time(skb_c, now);

	/* Dequeue according to the highest sojourn time */
	if (skb_l && (!skb_c || q->vars.dequeued_l < q->params.dequeue_ratio)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
		skb = __skb_dequeue(&q->l_queue->q);
#else
		skb = __qdisc_dequeue_head(&q->l_queue->q);

#endif
		skb_c = NULL;
		q->vars.dequeued_l += 1;
	} else if (skb_c) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
		skb = __skb_dequeue(&sch->q);
#else
		skb = __qdisc_dequeue_head(&sch->q);

#endif
		skb_l = NULL;
		q->vars.dequeued_l = 0;
	} else {
		/* Reset the WRR counter as all queues are empty */
		q->vars.dequeued_l = 0;
		goto drop;
	}

	qdisc_qstats_backlog_dec(sch, skb);
	if (skb_l) {
		/* We only decreased statistics in the internal queue, ensure
		 * that the exposed qdisc qlen reflects the changes.
		 */
		sch->q.qlen--;
		qdisc_qstats_backlog_dec(q->l_queue, skb);
	}

	/* drop on dequeue */
	if (!q->params.drop_early &&
	    should_drop(sch, q, get_ecn_field(skb), skb)) {
		testbed_inc_drop_count(&q->testbed, skb);
		q->vars.deferred_drop_count += 1;
		q->vars.deferred_drop_len += qdisc_pkt_len(skb);
		consume_skb(skb);
		q->stats.dropped++;
		qdisc_qstats_drop(sch);
		/* try next packet */
		goto pick_packet;
	}

	if (skb_l) {
		qlen_l = qdisc_qlen(q->l_queue);
		if (!q->params.et_packets ?
		/* to us; at least still one packet in the queue */
		(qdelay_l >> 10 > q->params.ecn_thresh) && qlen_l > 0 :
		qlen_l > q->params.ecn_thresh) {
			if (get_ecn_field(skb) != INET_ECN_CE
			    && INET_ECN_set_ce(skb))
				/* ECN threshold is exceeded, always mark */
				q->stats.ecn_mark++;
		}
		qdisc_bstats_update(q->l_queue, skb);
	}

drop:
	/* We cant call qdisc_tree_reduce_backlog() if our qlen is 0,
	 * or HTB crashes.
	 */
	if (q->vars.deferred_drop_count && sch->q.qlen) {
		qdisc_tree_reduce_backlog(sch, q->vars.deferred_drop_count,
					  q->vars.deferred_drop_len);
		q->vars.deferred_drop_count = 0;
		q->vars.deferred_drop_len = 0;
	}
	if (skb)
	{
		testbed_add_metrics(skb, &q->testbed,
				    (ktime_get_ns() - get_ts_cb(skb)) >> 10);
		qdisc_bstats_update(sch, skb);
	}

	return skb;
}

static void dualpi2_reset(struct Qdisc *sch)
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);

	qdisc_reset_queue(sch);
	qdisc_reset_queue(q->l_queue);
}

static void dualpi2_destroy(struct Qdisc *sch)
{
	struct dualpi2_sched_data *q = qdisc_priv(sch);

	q->params.tupdate = 0;
	del_timer_sync(&q->adapt_timer);
	if (q->l_queue != &noop_qdisc)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
		qdisc_destroy(q->l_queue);
#else
		qdisc_put(q->l_queue);
#endif
}

static struct Qdisc_ops dualpi2_qdisc_ops __read_mostly = {
	.id = "dualpi2",
	.priv_size	= sizeof(struct dualpi2_sched_data),
	.enqueue	= dualpi2_qdisc_enqueue,
	.dequeue	= dualpi2_qdisc_dequeue,
	.peek		= qdisc_peek_dequeued,
	.init		= dualpi2_init,
	.destroy	= dualpi2_destroy,
	.reset		= dualpi2_reset,
	.change		= dualpi2_change,
	.dump		= dualpi2_dump,
	.dump_stats	= dualpi2_dump_stats,
	.owner		= THIS_MODULE,
};

static int __init dualpi2_module_init(void)
{
	return register_qdisc(&dualpi2_qdisc_ops);
}

static void __exit dualpi2_module_exit(void)
{
	unregister_qdisc(&dualpi2_qdisc_ops);
}

module_init(dualpi2_module_init);
module_exit(dualpi2_module_exit);

MODULE_DESCRIPTION("Dual Queue - Proportional Integral controller "
		   "Improved with a Square (PI2) scheduler");
MODULE_AUTHOR("Koen De Schepper");
MODULE_AUTHOR("Olga Albisser");
MODULE_AUTHOR("Henrik Steen");
MODULE_AUTHOR("Olivier Tilmans");
MODULE_LICENSE("GPL");
