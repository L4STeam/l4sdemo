#ifndef __COMPAT_LINUX_H_
#define __COMPAT_LINUX_H_

#include <linux/version.h>

#include "compat-pkt_sched.h"

#include "../../../common/testbed.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
/* Workaround for missing backports of qdisc_tree_reduce_backlog which was
 * introduced in 4.6
 *
 * It is backported to 3.18.37, 4.1.28, 4.4.11, 4.5.5 and custom backports
 * such as ubuntu 4.2.0-39.46 which makes it difficult to check by looking at
 * LINUX_VERSION_CODE
 */
void qdisc_tree_reduce_backlog(struct Qdisc *sch, unsigned int n,
			       unsigned int len) __attribute__((weak));
void qdisc_tree_decrease_qlen(struct Qdisc *sch,
			      unsigned int n) __attribute__((weak));
#define qdisc_tree_reduce_backlog(_a,_b,_c) (qdisc_tree_reduce_backlog \
				? qdisc_tree_reduce_backlog(_a,_b,_c) \
				: qdisc_tree_decrease_qlen(_a,_b))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
static inline __be16 tc_skb_protocol(const struct sk_buff *skb)
{
	/* The real function check for the vlan accelerated path tag,
	 * cba portig that back as well.
	 */
	return skb->protocol;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
#define EMPTY()
#define DEFER(x) x EMPTY()
/* We need some pre-processor magic here to ensure the 2-args qdisc_drop is
 * not expanded as a macro but instead kept as a function
 */
#define rtnl_qdisc_drop(skb, sch) DEFER(qdisc_drop)(skb, sch)
#define qdisc_drop(skb, sch, to_free) DEFER(qdisc_drop)(skb, sch)
#define dualpi2_qdisc_enqueue(skb, sch, to_free) dualpi2_qdisc_enqueue(skb, sch)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
#define __qdisc_dequeue_head(x) __skb_dequeue(x)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
static inline int skb_try_make_writable(struct sk_buff *skb,
					unsigned int write_len)
{
	return skb_cloned(skb) && !skb_clone_writable(skb, write_len) &&
	       pskb_expand_head(skb, 0, 0, GFP_ATOMIC);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#define dualpi2_timer(timer_list) dualpi2_timer(unsigned long arg)
#define from_timer(q, timer, field) qdisc_priv((struct Qdisc *)arg)
#define timer_setup(timer, func, flag) \
	setup_timer(timer, func, (unsigned long)sch)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#define dualpi2_change(sch, opt, extack) dualpi2_change(sch, opt)
#define qdisc_create_dflt(dev, ops, handle, extack) \
	qdisc_create_dflt(dev, ops, handle)
#ifdef NL_SET_ERR_MSG_ATTR
#undef NL_SET_ERR_MSG_ATTR
#endif
#define NL_SET_ERR_MSG_ATTR(extack, rta, msg)
#define dualpi2_init(sch, opt, extack) dualpi2_init(sch, opt)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#define qdisc_put(x) qdisc_destroy(x)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
#define nla_nest_start_noflag(skb, flag) nla_nest_start(skb, flag)

	#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#define nla_parse_nested_deprecated(tb, max, rta, pol, extack) \
	nla_parse_nested(tb, max, rta, pol)
	#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#define nla_parse_nested_deprecated(tb, max, rta, pol, extack) \
	nla_parse_nested(tb, max, rta, pol, NULL)
	#else
#define nla_parse_nested_deprecated(tb, max, rta, pol, extack) \
	nla_parse_nested(tb, max, rta, pol, extack)
	#endif

#endif // LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)

#endif // __COMPAT_LINUX_H_
