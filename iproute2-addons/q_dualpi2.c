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
#include <errno.h>

#include "utils.h"
#include "tc_util.h"

#define MAX_PROB ((uint32_t)(~((uint32_t)0)))
#define DEFAULT_ALPHA_BETA ((uint32_t)(~((uint32_t)0)))
#define ALPHA_BETA_MAX 40000 /* Avoid overflows when computing PI2 probability*/
#define ALPHA_BETA_MIN 0
#define ALPHA_BETA_SCALE (1 << 8)
#define ALPHA_BETA_MIN_ENABLED ((float)1.0 / ALPHA_BETA_SCALE)


enum {
	INET_ECN_NOT_ECT = 0,
	INET_ECN_ECT_1 = 1,
	INET_ECN_ECT_0 = 2,
	INET_ECN_CE = 3,
	INET_ECN_MASK = 3,
};

static const char *get_ecn_type(uint8_t ect)
{
	switch (ect & INET_ECN_MASK) {
		case INET_ECN_ECT_1: return "l4s_ect";
		case INET_ECN_ECT_0:
		case INET_ECN_MASK: return "any_ect";
		default:
			fprintf(stderr,
				"Warning: Unexpected ecn type %u!\n", ect);
			return "";
	}
}

static void explain(void)
{
	fprintf(stderr, "Usage: ... dualpi2\n");
	fprintf(stderr, "               [limit PACKETS]\n");
	fprintf(stderr, "               [coupling_factor NUMBER]\n");
	fprintf(stderr, "               [target TIME] [tupdate TIME]\n");
	fprintf(stderr, "               [alpha ALPHA] [beta BETA]\n");
	fprintf(stderr, "               [step_thresh TIME|PACKETS]\n");
	fprintf(stderr, "               [drop_on_overload|overflow]\n");
	fprintf(stderr, "               [drop_enqueue|drop_dequeue]\n");
	fprintf(stderr, "               [classic_protection PERCENTAGE]\n");
}

static int get_float_in_range(float *val, const char *arg, float min, float max)
{
        float res;
        char *ptr;

        if (!arg || !*arg)
                return -1;
        res = strtof(arg, &ptr);
        if (!ptr || ptr == arg || *ptr)
                return -1;
	if (res < min || res > max)
		return -1;
        *val = res;
        return 0;
}

static int get_packets(uint32_t *val, const char *arg)
{
	unsigned long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtoul(arg, &ptr, 10);
	if (!ptr || ptr == arg ||
	    (strcmp(ptr, "pkt") && strcmp(ptr, "packet") &&
	     strcmp(ptr, "packets")))
		return -1;
	if (res == ULONG_MAX && errno == ERANGE)
		return -1;
	if (res > 0xFFFFFFFFUL)
		return -1;
	*val = res;
	return 0;
}

static int parse_alpha_beta(int argc, char **argv, uint32_t *field)
{

	float field_f;
	char *name = *argv;

	NEXT_ARG();
	if (get_float_in_range(&field_f, *argv, ALPHA_BETA_MIN,
			       ALPHA_BETA_MAX)) {
		fprintf(stderr, "Illegal \"%s\"\n", name);
		return -1;
	}
	else if (field_f < ALPHA_BETA_MIN_ENABLED)
		fprintf(stderr,
			"Warning: \"%s\" is too small and "
			"will be rounded to zero. Integral "
			"controller will be disabled\n", name);
	*field = (uint32_t)(field_f * ALPHA_BETA_SCALE);
	return 0;
}

static int try_get_percentage(int *val, const char *arg, int base)
{
	long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtol(arg, &ptr, base);
	if (!ptr || ptr == arg || (*ptr && strcmp(ptr, "%")))
		return -1;
	if (res == ULONG_MAX && errno == ERANGE)
		return -1;
	if (res < 0 || res > 100)
		return -1;

	*val = res;
	return 0;
}


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
	uint32_t limit = 0;
	uint32_t target = 0;
	uint32_t tupdate = 0;
	uint32_t alpha = DEFAULT_ALPHA_BETA;
	uint32_t beta = DEFAULT_ALPHA_BETA;
	int32_t coupling_factor = -1;
	uint8_t ecn_mask = INET_ECN_NOT_ECT;
	bool step_packets = false;
	uint32_t step_thresh = 0;
	int c_protection = -1;
	int drop_early = -1;
	int drop_overload = -1;
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_u32(&limit, *argv, 10)) {
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
		} else if (strcmp(*argv, "alpha") == 0 &&
			   parse_alpha_beta(argc, argv, &alpha))
				return -1;
		else if (strcmp(*argv, "beta") == 0 &&
			   parse_alpha_beta(argc, argv, &beta))
				return -1;
		else if (strcmp(*argv, "coupling_factor") == 0) {
			NEXT_ARG();
			if (get_s32(&coupling_factor, *argv, 0) ||
			    coupling_factor > 0xFFUL ||coupling_factor < 0) {
				fprintf(stderr,
					"Illegal \"coupling_factor\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "l4s_ect") == 0)
			ecn_mask = INET_ECN_ECT_1;
		else if (strcmp(*argv, "any_ect") == 0)
			ecn_mask = INET_ECN_MASK;
		else if (strcmp(*argv, "step_thresh") == 0) {
			NEXT_ARG();
			/* First assume that this is specified in time */
			if (get_time(&step_thresh, *argv)) {
				/* Then packets */
				if (get_packets(&step_thresh, *argv)) {
					fprintf(stderr,
						"Illegal \"step_thresh\"\n");
					return -1;
				}
				step_packets = true;
			}
		} else if (strcmp(*argv, "overflow") == 0) {
                        drop_overload = 0;
		} else if (strcmp(*argv, "drop_on_overload") == 0) {
                        drop_overload = 1;
		} else if (strcmp(*argv, "drop_enqueue") == 0) {
			drop_early = 1;
		} else if (strcmp(*argv, "drop_dequeue") == 0) {
			drop_early = 0;
		} else if (strcmp(*argv, "classic_protection") == 0) {
                        NEXT_ARG();
                        if (try_get_percentage(&c_protection, *argv, 10) ||
			    c_protection > 100 ||
			    c_protection < 0) {
                                fprintf(stderr,
					"Illegal \"classic_protection\"\n");
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
		--argc;
		++argv;
	}

	tail = NLMSG_TAIL(n);
	addattr_l(n, 1024, TCA_OPTIONS, NULL, 0);
	if (limit)
		addattr32(n, 1024, TCA_DUALPI2_LIMIT, limit);
	if (tupdate)
		addattr32(n, 1024, TCA_DUALPI2_TUPDATE, tupdate);
	if (target)
		addattr32(n, 1024, TCA_DUALPI2_TARGET, target);
	if (alpha != DEFAULT_ALPHA_BETA)
		addattr32(n, 1024, TCA_DUALPI2_ALPHA, alpha);
	if (beta != DEFAULT_ALPHA_BETA)
		addattr32(n, 1024, TCA_DUALPI2_BETA, beta);
	if (ecn_mask != INET_ECN_NOT_ECT)
		addattr8(n, 1024, TCA_DUALPI2_ECN_MASK, ecn_mask);
	if (drop_overload != -1)
		addattr8(n, 1024, TCA_DUALPI2_DROP_OVERLOAD, drop_overload);
	if (coupling_factor != -1)
		addattr8(n, 1024, TCA_DUALPI2_COUPLING, coupling_factor);
	if (step_thresh) {
		addattr32(n, 1024, TCA_DUALPI2_STEP_THRESH, step_thresh);
                addattr8(n, 1024, TCA_DUALPI2_STEP_PACKETS, step_packets);
	}
	if (drop_early != -1)
		addattr8(n, 1024, TCA_DUALPI2_DROP_EARLY, drop_early);
	if (c_protection != -1)
		addattr8(n, 1024, TCA_DUALPI2_C_PROTECTION, c_protection);

	tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;
	return 0;
}

static int dualpi2_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_DUALPI2_MAX + 1];
	uint32_t tupdate;
	uint32_t target;
	uint32_t step_thresh;
	bool step_packets = false;
	SPRINT_BUF(b1);

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_DUALPI2_MAX, opt);

	if (tb[TCA_DUALPI2_LIMIT] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_LIMIT]) >= sizeof(__uint32_t))
		fprintf(f, "limit %up ",
			rta_getattr_u32(tb[TCA_DUALPI2_LIMIT]));
	if (tb[TCA_DUALPI2_TARGET] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_TARGET]) >= sizeof(__uint32_t)) {
		target = rta_getattr_u32(tb[TCA_DUALPI2_TARGET]);
		fprintf(f, "target %s ", sprint_time(target, b1));
	}
	if (tb[TCA_DUALPI2_TUPDATE] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_TUPDATE]) >= sizeof(__uint32_t)) {
		tupdate = rta_getattr_u32(tb[TCA_DUALPI2_TUPDATE]);
		fprintf(f, "tupdate %s ", sprint_time(tupdate, b1));
	}
	if (tb[TCA_DUALPI2_ALPHA] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_ALPHA]) >= sizeof(__uint32_t)) {
		fprintf(f, "alpha %f ",
			(float)rta_getattr_u32(tb[TCA_DUALPI2_ALPHA]) /
			ALPHA_BETA_SCALE);
	}
	if (tb[TCA_DUALPI2_BETA] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_BETA]) >= sizeof(__uint32_t)) {
		fprintf(f, "beta %f ",
			(float)rta_getattr_u32(tb[TCA_DUALPI2_BETA]) /
			ALPHA_BETA_SCALE);
	}
	if (tb[TCA_DUALPI2_ECN_MASK] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_ECN_MASK]) >= sizeof(__u8))
		fprintf(f, "%s ",
			get_ecn_type(rta_getattr_u8(tb[TCA_DUALPI2_ECN_MASK])));
	if (tb[TCA_DUALPI2_COUPLING] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_COUPLING]) >= sizeof(__u8))
		fprintf(f, "coupling_factor %u ",
			rta_getattr_u8(tb[TCA_DUALPI2_COUPLING]));
	if (tb[TCA_DUALPI2_DROP_OVERLOAD] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_DROP_OVERLOAD]) >= sizeof(__u8)) {
		if (rta_getattr_u8(tb[TCA_DUALPI2_DROP_OVERLOAD]))
			fprintf(f, "drop_on_overload ");
		else
			fprintf(f, "overflow ");
	}
	if (tb[TCA_DUALPI2_STEP_PACKETS] &&
            RTA_PAYLOAD(tb[TCA_DUALPI2_STEP_PACKETS]) >= sizeof(__u8) &&
	    rta_getattr_u8(tb[TCA_DUALPI2_STEP_PACKETS]))
                        step_packets = true;
	if (tb[TCA_DUALPI2_STEP_THRESH] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_STEP_THRESH]) >= sizeof(__uint32_t)) {
		step_thresh = rta_getattr_u32(tb[TCA_DUALPI2_STEP_THRESH]);
		if (step_packets)
			fprintf(f, "step_thresh %upkt ", step_thresh);
		else
			fprintf(f, "step_thresh %s ",
				sprint_time(step_thresh, b1));
	}
	if (tb[TCA_DUALPI2_DROP_EARLY] &&
	    RTA_PAYLOAD(tb[TCA_DUALPI2_DROP_EARLY]) >= sizeof(__u8)) {
		if (rta_getattr_u8(tb[TCA_DUALPI2_DROP_EARLY]))
			fprintf(f, "drop_enqueue ");
		else
			fprintf(f, "drop_dequeue ");
	}
	if (tb[TCA_DUALPI2_C_PROTECTION] &&
            RTA_PAYLOAD(tb[TCA_DUALPI2_C_PROTECTION]) >= sizeof(__u8))
                fprintf(f, "classic_protection %u%% ",
			rta_getattr_u8(tb[TCA_DUALPI2_C_PROTECTION]));

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
	fprintf(f, "prob %f delay_c %uus delay_l %uus\n",
		(double)st->prob / (double)MAX_PROB, st->delay_c, st->delay_l);
	fprintf(f, "pkts_in_c %u pkts_in_l %u maxq %u\n",
		st->packets_in_c, st->packets_in_l, st->maxq);
	fprintf(f, "ecn_mark %u step_marks %u\n", st->ecn_mark, st->step_marks);
	fprintf(f, "credit %d (%c)\n", st->credit, st->credit > 0 ? 'C' : 'L');
	return 0;

}

struct qdisc_util dualpi2_qdisc_util = {
	.id = "dualpi2",
	.parse_qopt	= dualpi2_parse_opt,
	.print_qopt	= dualpi2_print_opt,
	.print_xstats	= dualpi2_print_xstats,
};
