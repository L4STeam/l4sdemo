/* Taildrop with stat collection derived from sch_fifo */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>

#include "testbed.h"

struct taildrop_skb_cb {
	u64 ts;
};

static inline struct taildrop_skb_cb *taildrop_skb_cb(struct sk_buff *skb)
{
	qdisc_cb_private_validate(skb, sizeof(struct taildrop_skb_cb));
	return (struct taildrop_skb_cb *)qdisc_skb_cb(skb)->data;
}

static inline struct testbed_metrics* taildrop_metrics(struct Qdisc *sch)
{
	return (struct testbed_metrics *)qdisc_priv(sch);
}

static int taildrop_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			    struct sk_buff **to_free)
{
	if (likely(sch->qstats.backlog + qdisc_pkt_len(skb) <= sch->limit)) {
		taildrop_skb_cb(skb)->ts = ktime_get_ns();
		return qdisc_enqueue_tail(skb, sch);
	}

	testbed_inc_drop_count(taildrop_metrics(sch), skb);
	return qdisc_drop(skb, sch, to_free);
}

static struct sk_buff *taildrop_dequeue(struct Qdisc *sch)
{
	struct sk_buff *skb = qdisc_dequeue_head(sch);
	if (likely(skb)) {
		u64 delay = (ktime_get_ns() - taildrop_skb_cb(skb)->ts) >> 10;

		testbed_add_metrics(skb, taildrop_metrics(sch), delay);
	}
	return skb;
}

static int taildrop_init(struct Qdisc *sch, struct nlattr *opt)
{
	testbed_metrics_init(taildrop_metrics(sch));
	if (opt == NULL) {
		u32 limit = qdisc_dev(sch)->tx_queue_len;

		sch->limit = limit * psched_mtu(qdisc_dev(sch));
	} else {
		struct tc_fifo_qopt *ctl = nla_data(opt);

		if (nla_len(opt) < sizeof(*ctl))
			return -EINVAL;

		sch->limit = ctl->limit;
	}
	return 0;
}

static int taildrop_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tc_fifo_qopt opt = { .limit = sch->limit };

	if (nla_put(skb, TCA_OPTIONS, sizeof(opt), &opt))
		goto nla_put_failure;
	return skb->len;

nla_put_failure:
	return -1;
}

struct Qdisc_ops taildrop_qdisc_ops __read_mostly = {
	.id		=	"taildrop",
	.priv_size	=	sizeof(struct testbed_metrics),
	.enqueue	=	taildrop_enqueue,
	.dequeue	=	taildrop_dequeue,
	.peek		=	qdisc_peek_head,
	.init		=	taildrop_init,
	.reset		=	qdisc_reset_queue,
	.change		=	taildrop_init,
	.dump		=	taildrop_dump,
	.owner		=	THIS_MODULE,
};

static int __init taildrop_module_init(void)
{
	return register_qdisc(&taildrop_qdisc_ops);
}

static void __exit taildrop_module_exit(void)
{
	unregister_qdisc(&taildrop_qdisc_ops);
}

module_init(taildrop_module_init);
module_exit(taildrop_module_exit);

MODULE_DESCRIPTION("Taildrop qdisc with stat collection");
MODULE_AUTHOR("Olivier Tilmans");
MODULE_LICENSE("GPL");
