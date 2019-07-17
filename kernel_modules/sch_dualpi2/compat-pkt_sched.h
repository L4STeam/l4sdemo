#ifndef __COMPAT_PKT_SCHED_H_
#define __COMPAT_PKT_SCHED_H_


/* Keep in sync with [linux tree]/include/uapi/linux/pkt_sched.h */
#ifndef TCA_DUALPI2_MAX


/* DUALPI2 */
enum {
	TCA_DUALPI2_UNSPEC,
	TCA_DUALPI2_LIMIT,		/* Packets */
	TCA_DUALPI2_TARGET,		/* us */
	TCA_DUALPI2_TUPDATE,		/* us */
	TCA_DUALPI2_ALPHA,		/* Hz scaled up by 256 */
	TCA_DUALPI2_BETA,		/* HZ scaled up by 256 */
	TCA_DUALPI2_STEP_THRESH,	/* Packets or us */
	TCA_DUALPI2_STEP_PACKETS,	/* Whether STEP_THRESH is in packets */
	TCA_DUALPI2_COUPLING,		/* Coupling factor between queues */
	TCA_DUALPI2_DROP_OVERLOAD,	/* Whether to drop on overload */
	TCA_DUALPI2_DROP_EARLY,		/* Whether to drop on enqueue */
	TCA_DUALPI2_C_PROTECTION,	/* Percentage */
	TCA_DUALPI2_ECN_MASK,		/* L4S queue classification mask */
	TCA_DUALPI2_PAD,
	__TCA_DUALPI2_MAX
};

#define TCA_DUALPI2_MAX   (__TCA_DUALPI2_MAX - 1)

struct tc_dualpi2_xstats {
	__u32 prob;             /* current probability */
	__u32 delay_c;		/* current delay in C queue */
	__u32 delay_l;		/* current delay in L queue */
	__s32 credit;		/* current c_protection credit */
	__u32 packets_in_c;	/* number of packets enqueued in C queue */
	__u32 packets_in_l;	/* number of packets enqueued in L queue */
	__u32 maxq;             /* maximum queue size */
	__u32 ecn_mark;         /* packets marked with ecn*/
};


#endif /* TCA_DUALPI2_MAX */

#endif
