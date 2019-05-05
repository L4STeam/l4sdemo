/* Copyright (C) 2015 Alcatel-Lucent.
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
 * PI Improved with a Square (PI2)
 * Supports controlling scalable congestion controls (DCTCP, etc...)
 * Supports DualQ with PI2
 * Supports L4S ECN identifier
 * Author: Koen De Schepper <koen.de_schepper@alcatel-lucent.com>
 * Author: Olga Bondarenko <olgabo@simula.no>
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

static void explain(void)
{
	fprintf(stderr, "Usage: ... pi2 [limit PACKETS] [target TIME us] [tupdate TIME us]\n");
	fprintf(stderr, "               [alpha ALPHA] [beta BETA] [bytemode|nobytemode] [sojourn|rate_estimate]\n");
	fprintf(stderr, "               [dualq_ect1|dualq|ecn|noecn] [k KFACTOR] [ect1_scal|ecn_scal|no_scal]\n");
	fprintf(stderr, "               [l_thresh TIME us] [t_shift TIME us] [l_drop PROBABILITY %%]\n");
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
#define ALPHA_MAX 32000
#define ALPHA_MIN 0
#define BETA_MAX 320000
#define BETA_MIN 0
#define K_MAX 64
#define K_MIN 1
#define P_MAX 100
#define P_MIN 1

static int pi2_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			 struct nlmsghdr *n)
{
	unsigned int limit   = 0;
	unsigned int target  = 0;
	unsigned int tupdate = 0;
	unsigned int alpha   = DEFAULT_ALPHA_BETA;
	unsigned int beta    = DEFAULT_ALPHA_BETA;
	unsigned int kfactor = 0;
	int ecn_scal = -1;
	unsigned int l_thresh = 0;
	unsigned int t_shift = 0;
	unsigned int l_drop = 0;
	int ecn = -1;
	int bytemode = -1;
	int sojourn = -1;
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
			if (get_unsigned(&alpha, *argv, 0) ||
			    (alpha > ALPHA_MAX) || (alpha < ALPHA_MIN)) {
				fprintf(stderr, "Illegal \"alpha\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "beta") == 0) {
			NEXT_ARG();
			if (get_unsigned(&beta, *argv, 0) ||
			    (beta > BETA_MAX) || (beta < BETA_MIN)) {
				fprintf(stderr, "Illegal \"beta\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "k") == 0) {
			NEXT_ARG();
			if (get_unsigned(&kfactor, *argv, 0) ||
			    (kfactor > K_MAX) || (kfactor < K_MIN)) {
				fprintf(stderr, "Illegal \"k\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "ecn_scal") == 0) { // all ect gets non-squared prob (is scalable)
			ecn_scal = 3;
		} else if (strcmp(*argv, "ect1_scal") == 0) { // only ect(1) gets non-squared prob (is scalable)
			ecn_scal = 1;
		} else if (strcmp(*argv, "no_scal") == 0) { // no ect gets non-squared prob (all is classic)
			ecn_scal = 0;
		} else if (strcmp(*argv, "l_thresh") == 0) {
			NEXT_ARG();
			if (get_time(&l_thresh, *argv)) {
				fprintf(stderr, "Illegal \"l_thresh\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "t_shift") == 0) {
			NEXT_ARG();
			if (get_time(&t_shift, *argv)) {
				fprintf(stderr, "Illegal \"t_shift\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "dualq") == 0) { // for now all ECT are in L4S Q
			ecn = 3;
		} else if (strcmp(*argv, "dualq_ect1") == 0) { // to become default: only ECT(1) in L4S Q
			ecn = 2;
		} else if (strcmp(*argv, "ecn") == 0) {
			ecn = 1;
		} else if (strcmp(*argv, "noecn") == 0) {
			ecn = 0;
		} else if (strcmp(*argv, "sojourn") == 0) {
			sojourn = 1;
		} else if (strcmp(*argv, "l_drop") == 0) {
			NEXT_ARG();
			if (get_unsigned(&l_drop, *argv, 0) ||
			    (l_drop > P_MAX) || (l_drop < P_MIN)) {
				fprintf(stderr, "Illegal \"l_drop\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "rate_estimate") == 0) {
			sojourn = 0;
		} else if (strcmp(*argv, "bytemode") == 0) {
			bytemode = 1;
		} else if (strcmp(*argv, "nobytemode") == 0) {
			bytemode = 0;
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

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	if (limit)
		addattr_l(n, 1024, TCA_PI2_LIMIT, &limit, sizeof(limit));
	if (tupdate)
		addattr_l(n, 1024, TCA_PI2_TUPDATE, &tupdate, sizeof(tupdate));
	if (target)
		addattr_l(n, 1024, TCA_PI2_TARGET, &target, sizeof(target));
	if (alpha != DEFAULT_ALPHA_BETA)
		addattr_l(n, 1024, TCA_PI2_ALPHA, &alpha, sizeof(alpha));
	if (beta != DEFAULT_ALPHA_BETA)
		addattr_l(n, 1024, TCA_PI2_BETA, &beta, sizeof(beta));
	if (ecn != -1)
		addattr_l(n, 1024, TCA_PI2_ECN, &ecn, sizeof(ecn));
	if (bytemode != -1)
		addattr_l(n, 1024, TCA_PI2_BYTEMODE, &bytemode,
			  sizeof(bytemode));
	if (sojourn != -1)
		addattr_l(n, 1024, TCA_PI2_SOJOURN, &sojourn,
			  sizeof(sojourn));
	if (l_drop)
		addattr_l(n, 1024, TCA_PI2_L_DROP, &l_drop,
			  sizeof(l_drop));
	if (kfactor)
		addattr_l(n, 1024, TCA_PI2_K, &kfactor, sizeof(kfactor));
	if (ecn_scal != -1)
		addattr_l(n, 1024, TCA_PI2_ECN_SCAL, &ecn_scal, sizeof(ecn_scal));
	if (l_thresh)
		addattr_l(n, 1024, TCA_PI2_L_THRESH, &l_thresh, sizeof(l_thresh));
	if (t_shift)
		addattr_l(n, 1024, TCA_PI2_T_SHIFT, &t_shift, sizeof(t_shift));

	tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;
	return 0;
}

static int pi2_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_PI2_MAX + 1];
	unsigned int limit;
	unsigned int tupdate;
	unsigned int target;
	unsigned int alpha;
	unsigned int beta;
	unsigned int ecn_scal;
	unsigned int l_thresh;
	unsigned int t_shift;
	unsigned ecn;
	unsigned bytemode;
	unsigned sojourn;
	unsigned int kfactor;
	unsigned int l_drop;
	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_PI2_MAX, opt);

	if (tb[TCA_PI2_LIMIT] &&
	    RTA_PAYLOAD(tb[TCA_PI2_LIMIT]) >= sizeof(__u32)) {
		limit = rta_getattr_u32(tb[TCA_PI2_LIMIT]);
		fprintf(f, "limit %up ", limit);
	}
	if (tb[TCA_PI2_TARGET] &&
	    RTA_PAYLOAD(tb[TCA_PI2_TARGET]) >= sizeof(__u32)) {
		target = rta_getattr_u32(tb[TCA_PI2_TARGET]);
		fprintf(f, "target %s ", sprint_time(target, b1));
	}
	if (tb[TCA_PI2_TUPDATE] &&
	    RTA_PAYLOAD(tb[TCA_PI2_TUPDATE]) >= sizeof(__u32)) {
		tupdate = rta_getattr_u32(tb[TCA_PI2_TUPDATE]);
		fprintf(f, "tupdate %s ", sprint_time(tupdate, b1));
	}
	if (tb[TCA_PI2_ALPHA] &&
	    RTA_PAYLOAD(tb[TCA_PI2_ALPHA]) >= sizeof(__u32)) {
		alpha = rta_getattr_u32(tb[TCA_PI2_ALPHA]);
		fprintf(f, "alpha %u ", alpha);
	}
	if (tb[TCA_PI2_BETA] &&
	    RTA_PAYLOAD(tb[TCA_PI2_BETA]) >= sizeof(__u32)) {
		beta = rta_getattr_u32(tb[TCA_PI2_BETA]);
		fprintf(f, "beta %u ", beta);
	}
	if (tb[TCA_PI2_ECN] && RTA_PAYLOAD(tb[TCA_PI2_ECN]) >= sizeof(__u32)) {
		ecn = rta_getattr_u32(tb[TCA_PI2_ECN]);
		if (ecn == 1)
			fprintf(f, "ecn ");
		else if (ecn == 2)
			fprintf(f, "dualq_ect1 ");
		else if (ecn == 3)
			fprintf(f, "dualq ");
	}
	if (tb[TCA_PI2_SOJOURN] &&
	    RTA_PAYLOAD(tb[TCA_PI2_SOJOURN]) >= sizeof(__u32)) {
		sojourn = rta_getattr_u32(tb[TCA_PI2_SOJOURN]);
		if (sojourn)
			fprintf(f, "sojourn ");
		else
			fprintf(f, "rate_estimate ");
	}
	if (tb[TCA_PI2_BYTEMODE] &&
	    RTA_PAYLOAD(tb[TCA_PI2_BYTEMODE]) >= sizeof(__u32)) {
		bytemode = rta_getattr_u32(tb[TCA_PI2_BYTEMODE]);
		if (bytemode)
			fprintf(f, "bytemode ");
	}
	if (tb[TCA_PI2_K] &&
	    RTA_PAYLOAD(tb[TCA_PI2_K]) >= sizeof(__u32)) {
		kfactor = rta_getattr_u32(tb[TCA_PI2_K]);
		fprintf(f, "k %u ", kfactor);
	}
	if (tb[TCA_PI2_L_DROP] &&
	    RTA_PAYLOAD(tb[TCA_PI2_L_DROP]) >= sizeof(__u32)) {
		l_drop = rta_getattr_u32(tb[TCA_PI2_L_DROP]);
		fprintf(f, "l_drop %u ", l_drop);
	}
	if (tb[TCA_PI2_ECN_SCAL] && RTA_PAYLOAD(tb[TCA_PI2_ECN_SCAL]) >= sizeof(__u32)) {
		ecn_scal = rta_getattr_u32(tb[TCA_PI2_ECN_SCAL]);
		if (ecn_scal == 0)
			fprintf(f, "no_scal ");
		else if (ecn_scal == 1)
			fprintf(f, "ect1_scal ");
		else if (ecn_scal == 3)
			fprintf(f, "ecn_scal ");
	}
	if (tb[TCA_PI2_L_THRESH] &&
	    RTA_PAYLOAD(tb[TCA_PI2_L_THRESH]) >= sizeof(__u32)) {
		l_thresh = rta_getattr_u32(tb[TCA_PI2_L_THRESH]);
		fprintf(f, "l_thresh %s ", sprint_time(l_thresh, b1));
	}
	if (tb[TCA_PI2_T_SHIFT] &&
	    RTA_PAYLOAD(tb[TCA_PI2_T_SHIFT]) >= sizeof(__u32)) {
		t_shift = rta_getattr_u32(tb[TCA_PI2_T_SHIFT]);
		fprintf(f, "t_shift %s ", sprint_time(t_shift, b1));
	}

	return 0;
}

static int pi2_print_xstats(struct qdisc_util *qu, FILE *f,
			    struct rtattr *xstats)
{
	struct tc_pie_xstats *st;

	if (xstats == NULL)
		return 0;

	if (RTA_PAYLOAD(xstats) < sizeof(*st))
		return -1;

	st = RTA_DATA(xstats);
	/*prob is returned as a fracion of maximum integer value */
	fprintf(f, "prob %f delay %uus avg_dq_rate %u\n",
		(double)st->prob / (double)0xffffffff, st->delay,
		st->avg_dq_rate);
	fprintf(f, "pkts_in %u overlimit %u dropped %u maxq %u ecn_mark %u\n",
		st->packets_in, st->overlimit, st->dropped, st->maxq,
		st->ecn_mark);
	return 0;

}

struct qdisc_util pi2_qdisc_util = {
	.id = "pi2",
	.parse_qopt	= pi2_parse_opt,
	.print_qopt	= pi2_print_opt,
	.print_xstats	= pi2_print_xstats,
};
