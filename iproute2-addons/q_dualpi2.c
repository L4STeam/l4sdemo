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
 * DualQ PI Improved with a Square (dualpi2)
 * Supports controlling scalable congestion controls (DCTCP, etc...)
 * Supports DualQ with PI2
 * Supports L4S ECN identifier
 * Author: Koen De Schepper <koen.de_schepper@nokia-bell-labs.com>
 * Author: Olga Albisser <olga@albisser.org>
 * Author: Henrik Steen <henrist@henrist.net>
 *
 * Based on the PIE implementation:
 * Copyright (C) 2013 Cisco Systems, Inc, 2013.
 * Author: Vijay Subramanian <vijaynsu@cisco.com>
 * Author: Mythili Prabhu <mysuryan@cisco.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "tc_util.h"

enum {
	INET_ECN_NOT_ECT = 0,
	INET_ECN_ECT_1 = 1,
	INET_ECN_CE = 3,
	INET_ECN_MASK = 3,
};

static void explain(void)
{
	fprintf(stderr, "Usage: ... dualpi2 [limit PACKETS] [target TIME] [tupdate TIME]\n");
	fprintf(stderr, "               [alpha ALPHA] [beta BETA]\n");
	fprintf(stderr, "               [no_dualq|l4s_dualq|dc_dualq] [k KFACTOR]\n");
	fprintf(stderr, "               [no_ecn|classic_ecn|l4s_ecn|dc_ecn]\n");
	fprintf(stderr, "               [et_packets|et_time] [l_thresh TIME | PACKETS]\n");
	fprintf(stderr, "		[c_limit | l_drop PROBABILITY %%]\n");
	fprintf(stderr, "               [drop_enqueue|drop_dequeue]\n");
	fprintf(stderr, "               [wrr_ratio PACKETS]\n");
}

static int get_float(float *val, const char *arg)
{
        float res;
        char *ptr;

        if (!arg || !*arg)
                return -1;
        res = strtof(arg, &ptr);
        if (!ptr || ptr == arg || *ptr)
                return -1;
        *val = res;
        return 0;
}

#define DEFAULT_ALPHA_BETA 0xffffffff
#define ALPHA_BETA_MAX 40000
#define ALPHA_BETA_MIN_ENABLED 0.00390625
#define ALPHA_BETA_MIN 0
#define K_MAX 8
#define K_MIN 1
#define P_MAX 100
#define P_MIN 0
#define T_SPEED_MAX 31

/* iproute2 v4.15 changed the API in commit b317557f5854bb8 / tag v4.15
 * Conveniently, the CBS scheduler got introduced in Linux in commit
 * 0f7787b4133fb / tag v4.15.
 * Checking for the presence of *CBS*-related defines in pkt_sched.h is thus
 * a sign that we are on iproute2 >= 4.15
 */
static int dualpi2_parse_opt(struct qdisc_util *qu, int argc, char **argv,
#ifdef TCA_CBS_MAX
			 struct nlmsghdr *n, const char* dev)
#else
			 struct nlmsghdr *n)
#endif
{
	unsigned int limit   = 0;
	unsigned int target  = 0;
	unsigned int tupdate = 0;
	unsigned int alpha   = DEFAULT_ALPHA_BETA;
	float alpha_f = 0;
	unsigned int beta    = DEFAULT_ALPHA_BETA;
	float beta_f = 0;
	unsigned int kfactor = 0;
	unsigned int et_packets = 0;
	unsigned int et_time = 0;
	unsigned int l_thresh = 0;
	unsigned int c_limit = 0;
	unsigned int l_drop = 0;
	unsigned int wrr_ratio = 0;
	int queue_mask = -1;
	int ecn = -1;
	int drop_early = -1;
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_unsigned(&limit, *argv, 0)) {
				fprintf(stderr, "Illegal \"limit\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "target") == 0) {
			NEXT_ARG();
			if (get_time(&target, *argv)) {
				fprintf(stderr, "Illegal \"target\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "tupdate") == 0) {
			NEXT_ARG();
			if (get_time(&tupdate, *argv)) {
				fprintf(stderr, "Illegal \"tupdate\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "alpha") == 0) {
			NEXT_ARG();
			if (get_float(&alpha_f, *argv) ||
			    (alpha_f > ALPHA_BETA_MAX) || (alpha_f < ALPHA_BETA_MIN)) {
				fprintf(stderr, "Illegal \"alpha\"\n");
				return -1;
			}
			if (alpha_f == 0) {
				fprintf(stderr, "Warning: \"alpha\" is zero. Integral controller will be disabled.\n");
			}
			else if (alpha_f < ALPHA_BETA_MIN_ENABLED) {
                                fprintf(stderr, "Warning: \"alpha\" is too small and will be rounded to zero. Integral controller will be disabled\n");
                        }
			alpha = (unsigned int)(alpha_f * 256);
		} else if (strcmp(*argv, "beta") == 0) {
			NEXT_ARG();
			if (get_float(&beta_f, *argv) ||
			    (beta_f > ALPHA_BETA_MAX) || (beta_f < ALPHA_BETA_MIN)) {
				fprintf(stderr, "Illegal \"beta\"\n");
				return -1;
			}
			if (beta_f == 0) {
                                fprintf(stderr, "Warning: \"beta\" is zero. Proportional controller will be disabled.\n");
			}
			else if (beta_f < ALPHA_BETA_MIN_ENABLED) {
                                fprintf(stderr, "Warning: \"beta\" is too small and will be rounded to zero. Proportional controller will be disabled.\n");
                        }
			beta = (unsigned int)(beta_f * 256);
		} else if (strcmp(*argv, "k") == 0) {
			NEXT_ARG();
			if (get_unsigned(&kfactor, *argv, 0) ||
			    (kfactor > K_MAX) || (kfactor < K_MIN)) {
				fprintf(stderr, "Illegal \"k\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "no_dualq") == 0) {
			queue_mask = INET_ECN_NOT_ECT;
		} else if (strcmp(*argv, "l4s_dualq") == 0) {
			queue_mask = INET_ECN_ECT_1;
		} else if (strcmp(*argv, "dc_dualq") == 0) {
			queue_mask = INET_ECN_CE;
		} else if (strcmp(*argv, "no_ecn") == 0) {
			ecn = INET_ECN_NOT_ECT;
		} else if (strcmp(*argv, "classic_ecn") == 0) {
			ecn = (INET_ECN_MASK << 2) | INET_ECN_NOT_ECT;
		} else if (strcmp(*argv, "l4s_ecn") == 0) {
			ecn = (INET_ECN_MASK << 2) | INET_ECN_ECT_1;
		} else if (strcmp(*argv, "dc_ecn") == 0) {
			ecn = (INET_ECN_MASK << 2) | INET_ECN_CE;
		} else if (strcmp(*argv, "et_packets") == 0) {
                        et_packets = 1;
                } else if (strcmp(*argv, "et_time") == 0) {
                        et_time = 1;
		} else if (strcmp(*argv, "l_thresh") == 0) {
			NEXT_ARG();
			if (get_time(&l_thresh, *argv)) {
				fprintf(stderr, "Illegal \"l_thresh\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "l_drop") == 0) {
			NEXT_ARG();
			if (get_unsigned(&l_drop, *argv, 0) ||
			    (l_drop > P_MAX) || (l_drop < P_MIN)) {
				fprintf(stderr, "Illegal \"l_drop\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "c_limit") == 0) {
                        c_limit = 1;
		} else if (strcmp(*argv, "drop_enqueue") == 0) {
			drop_early = 1;
		} else if (strcmp(*argv, "drop_dequeue") == 0) {
			drop_early = 0;
		} else if (strcmp(*argv, "wrr_ratio") == 0) {
                        NEXT_ARG();
                        if (get_unsigned(&wrr_ratio, *argv, 0)) {
                                fprintf(stderr, "Illegal \"wrr_ratio\"\n");
                                return -1;
                        }
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "What is \"%s\"?\n", *argv);
			explain();
			return -1;
		}
		argc--;
		argv++;
	}

	if (c_limit && l_drop) {
		fprintf(stderr, "c_limit cannot be used with l_drop, use either c_limit or l_drop (refer to README)");
                explain();
                return -1;
	}

	if (et_packets && et_time) {
                fprintf(stderr, "et_packets cannot be used with et_time, use either et_packets or et_time (refer to README)");
                explain();
                return -1;
        }

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	if (limit)
		addattr_l(n, 1024, TCA_DUALPI2_LIMIT, &limit, sizeof(limit));
	if (tupdate)
		addattr_l(n, 1024, TCA_DUALPI2_TUPDATE, &tupdate, sizeof(tupdate));
	if (target)
		addattr_l(n, 1024, TCA_DUALPI2_TARGET, &target, sizeof(target));
	if (alpha != DEFAULT_ALPHA_BETA)
		addattr_l(n, 1024, TCA_DUALPI2_ALPHA, &alpha, sizeof(alpha));
	if (beta != DEFAULT_ALPHA_BETA)
		addattr_l(n, 1024, TCA_DUALPI2_BETA, &beta, sizeof(beta));
	if (queue_mask != -1)
		addattr_l(n, 1024, TCA_DUALPI2_DUALQ, &queue_mask, sizeof(queue_mask));
	if (ecn != -1)
		addattr_l(n, 1024, TCA_DUALPI2_ECN, &ecn, sizeof(ecn));
	if (l_drop)
		addattr_l(n, 1024, TCA_DUALPI2_L_DROP, &l_drop,
			  sizeof(l_drop));
	if (kfactor)
		addattr_l(n, 1024, TCA_DUALPI2_K, &kfactor, sizeof(kfactor));
	if (et_packets)
                addattr_l(n, 1024, TCA_DUALPI2_ET_PACKETS, &et_packets, sizeof(et_packets));
	if (l_thresh)
		addattr_l(n, 1024, TCA_DUALPI2_L_THRESH, &l_thresh, sizeof(l_thresh));
	if (drop_early != -1)
		addattr_l(n, 1024, TCA_DUALPI2_DROP_EARLY, &drop_early,
			  sizeof(drop_early));
	if (wrr_ratio)
		addattr_l(n, 1024, TCA_DUALPI2_WRR_RATIO, &wrr_ratio, sizeof(wrr_ratio));

	tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;
	return 0;
}

static int dualpi2_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_DUALPI2_MAX + 1];
	unsigned int limit;
	unsigned int tupdate;
	unsigned int target;
	unsigned int alpha;
	float alpha_f;
	unsigned int beta;
	float beta_f;
	unsigned int queue_mask;
	unsigned int ecn;
	unsigned int et_packets;
	unsigned int l_thresh;
	unsigned int kfactor;
	unsigned int l_drop;
	unsigned int drop_early;
	unsigned int wrr_ratio;
	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_DUALPI2_MAX, opt);

	if (tb[TCA_DUALPI2_LIMIT] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_DUALPI2_LIMIT]);
		fprintf(f, "limit %up ", limit);
	}
	if (tb[TCA_DUALPI2_TARGET] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_TARGET]) >= sizeof(__u32)) {
		target = rta_getattr_u32(tb[TCA_DUALPI2_TARGET]);
		fprintf(f, "target %s ", sprint_time(target, b1));
	}
	if (tb[TCA_DUALPI2_TUPDATE] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_TUPDATE]) >= sizeof(__u32)) {
		tupdate = rta_getattr_u32(tb[TCA_DUALPI2_TUPDATE]);
		fprintf(f, "tupdate %s ", sprint_time(tupdate, b1));
	}
	if (tb[TCA_DUALPI2_ALPHA] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_ALPHA]) >= sizeof(__u32)) {
		alpha = rta_getattr_u32(tb[TCA_DUALPI2_ALPHA]);
		alpha_f = (float) alpha / 256;
		fprintf(f, "alpha %f ", alpha_f);
	}
	if (tb[TCA_DUALPI2_BETA] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_BETA]) >= sizeof(__u32)) {
		beta = rta_getattr_u32(tb[TCA_DUALPI2_BETA]);
		beta_f = (float) beta / 256;
		fprintf(f, "beta %f ", beta_f);
	}
	if (tb[TCA_DUALPI2_DUALQ] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_DUALQ]) >= sizeof(__u32)) {
		queue_mask = rta_getattr_u32(tb[TCA_DUALPI2_DUALQ]);
		if (queue_mask == INET_ECN_NOT_ECT)
			fprintf(f, "no_dualq ");
		else if (queue_mask == INET_ECN_ECT_1)
			fprintf(f, "l4s_dualq ");
		else if (queue_mask == INET_ECN_CE)
			fprintf(f, "dc_dualq ");
	}
	if (tb[TCA_DUALPI2_ECN] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_ECN]) >= sizeof(__u32)) {
		ecn = rta_getattr_u32(tb[TCA_DUALPI2_ECN]);
		if (ecn == INET_ECN_NOT_ECT)
			fprintf(f, "no_ecn ");
		else if (ecn == (INET_ECN_MASK << 2))
			fprintf(f, "classic_ecn ");
		else if (ecn == ((INET_ECN_MASK << 2) | 1))
			fprintf(f, "l4s_ecn ");
		else if (ecn == ((INET_ECN_MASK << 2) | INET_ECN_CE))
			fprintf(f, "dc_ecn ");
	}
	if (tb[TCA_DUALPI2_K] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_K]) >= sizeof(__u32)) {
		kfactor = rta_getattr_u32(tb[TCA_DUALPI2_K]);
		fprintf(f, "k %u ", kfactor);
	}
	if (tb[TCA_DUALPI2_L_DROP] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_L_DROP]) >= sizeof(__u32)) {
		l_drop = rta_getattr_u32(tb[TCA_DUALPI2_L_DROP]);
		if (l_drop > 0)
			fprintf(f, "l_drop %u ", l_drop);
		else
			fprintf(f, "c_limit ");
	}
	if (tb[TCA_DUALPI2_ET_PACKETS] &&
            RTA_PAYLOAD(tb[TCA_DUALPI2_ET_PACKETS]) >= sizeof(__u32)) {
                et_packets = rta_getattr_u32(tb[TCA_DUALPI2_ET_PACKETS]);
                if (et_packets > 0)
                        fprintf(f, "et_packets ");
                else
                        fprintf(f, "et_time ");
        }
	if (tb[TCA_DUALPI2_L_THRESH] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_L_THRESH]) >= sizeof(__u32)) {
		l_thresh = rta_getattr_u32(tb[TCA_DUALPI2_L_THRESH]);
		fprintf(f, "l_thresh %s ", sprint_time(l_thresh, b1));
	}
	if (tb[TCA_DUALPI2_DROP_EARLY] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_DROP_EARLY]) >= sizeof(__u32)) {
		drop_early = rta_getattr_u32(tb[TCA_DUALPI2_DROP_EARLY]);
		if (drop_early)
			fprintf(f, "drop_enqueue ");
		else
			fprintf(f, "drop_dequeue ");
	}
	if (tb[TCA_DUALPI2_WRR_RATIO] &&
            RTA_PAYLOAD(tb[TCA_DUALPI2_WRR_RATIO]) >= sizeof(__u32)) {
                wrr_ratio = rta_getattr_u32(tb[TCA_DUALPI2_WRR_RATIO]);
                fprintf(f, "wrr_ratio %u ", wrr_ratio);
        }

	return 0;
}

static int dualpi2_print_xstats(struct qdisc_util *qu, FILE *f,
			    struct rtattr *xstats)
{
	struct tc_dualpi2_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);
	/* prob is returned as a fracion of maximum integer value */
	fprintf(f, "prob %f delay_c %uus delay_l %uus\n",
		(double)st->prob / (double)0xffffffff, st->delay_c, st->delay_l);
	fprintf(f, "pkts_in %u overlimit %u dropped %u maxq %u ecn_mark %u\n",
		st->packets_in, st->overlimit, st->dropped, st->maxq,
		st->ecn_mark);
	return 0;

}

struct qdisc_util dualpi2_qdisc_util = {
	.id = "dualpi2",
	.parse_qopt	= dualpi2_parse_opt,
	.print_qopt	= dualpi2_print_opt,
	.print_xstats	= dualpi2_print_xstats,
};
