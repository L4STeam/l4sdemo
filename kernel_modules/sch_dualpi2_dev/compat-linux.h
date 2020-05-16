#ifndef __COMPAT_LINUX_H_
#define __COMPAT_LINUX_H_

#include <linux/version.h>

#include "compat-pkt_sched.h"

#include "../../common/testbed.h" /* only used for testbed */

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

#endif
