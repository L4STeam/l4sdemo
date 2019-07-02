#ifndef __COMPAT_PKT_SCHED_H_
#define __COMPAT_PKT_SCHED_H_


/* Keep in sync with [linux tree]/include/uapi/linux/pkt_sched.h */
#ifndef TCA_DUALPI2_MAX


/* DUALPI2 */
enum {
	TCA_DUALPI2_UNSPEC,
	TCA_DUALPI2_ALPHA,
	TCA_DUALPI2_BETA,
	TCA_DUALPI2_DUALQ,
	TCA_DUALPI2_ECN,
	TCA_DUALPI2_K,
	TCA_DUALPI2_L_DROP,
	TCA_DUALPI2_ET_PACKETS,
	TCA_DUALPI2_L_THRESH,
	TCA_DUALPI2_LIMIT,
	TCA_DUALPI2_TARGET,
	TCA_DUALPI2_TUPDATE,
	TCA_DUALPI2_DROP_EARLY,
	TCA_DUALPI2_WRR_RATIO,
	TCA_DUALPI2_PAD,
	__TCA_DUALPI2_MAX
};

#define TCA_DUALPI2_MAX   (__TCA_DUALPI2_MAX - 1)

struct tc_dualpi2_xstats {
	__u32 prob;             /* current probability */
	__u32 delay_c;		/* current delay in C queue */
	__u32 delay_l;		/* current delay in L queue */
	__u32 packets_in;       /* total number of packets enqueued */
	__u32 dropped;          /* packets dropped due to pie_action */
	__u32 overlimit;        /* dropped due to lack of space in queue */
	__u32 maxq;             /* maximum queue size */
	__u32 ecn_mark;         /* packets marked with ecn*/
};


#endif /* TCA_DUALPI2_MAX */

#endif
