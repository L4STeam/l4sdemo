/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2019 Nokia.
 *
 * DualQ PI Improved with a Square (dualpi2)
 * Supports controlling scalable congestion controls (DCTCP, etc...)
 * Supports DualQ with PI2
 * Supports L4S ECN identifier
 * Author: Koen De Schepper <koen.de_schepper@nokia-bell-labs.com>
 * Author: Olga Albisser <olga@albisser.org>
 * Author: Henrik Steen <henrist@henrist.net>
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
#define ALPHA_BETA_MAX ((2 << 23) - 1) /* see net/sched/sch_dualpi2.c */
#define ALPHA_BETA_SCALE (1 << 8)
#define RTT_TYP_TO_MAX 6

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
	fprintf(stderr, "               [step_thresh TIME|PACKETS]\n");
	fprintf(stderr, "               [drop_on_overload|overflow]\n");
	fprintf(stderr, "               [drop_enqueue|drop_dequeue]\n");
	fprintf(stderr, "               [classic_protection PERCENTAGE]\n");
	fprintf(stderr, "               [max_rtt TIME [typical_rtt TIME]]\n");
	fprintf(stderr, "               [target TIME] [tupdate TIME]\n");
	fprintf(stderr, "               [alpha ALPHA] [beta BETA]\n");
}

static int get_float(float *val, const char *arg, float min, float max)
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

static int parse_alpha_beta(const char *name, char *argv, uint32_t *field)
{

	float field_f;

	if (get_float(&field_f, argv, 0.0, ALPHA_BETA_MAX)) {
		fprintf(stderr, "Illegal \"%s\"\n", name);
		return -1;
	}
	else if (field_f < 1.0f / ALPHA_BETA_SCALE)
		fprintf(stderr, "Warning: \"%s\" is too small and will be "
			"rounded to zero.\n", name);
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
	uint32_t rtt_max = 0;
	uint32_t rtt_typ = 0;
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
		} else if (strcmp(*argv, "alpha") == 0) {
			NEXT_ARG();
			if (parse_alpha_beta("alpha", *argv, &alpha))
				return -1;
		} else if (strcmp(*argv, "beta") == 0) {
			NEXT_ARG();
			if (parse_alpha_beta("beta", *argv, &beta))
				return -1;
		} else if (strcmp(*argv, "coupling_factor") == 0) {
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
		} else if (strcmp(*argv, "max_rtt") == 0) {
			NEXT_ARG();
			if (get_time(&rtt_max, *argv)) {
				fprintf(stderr, "Illegal \"rtt_max\"\n");
				return -1;
			}
		} else if (strcmp(*argv, "typical_rtt") == 0) {
			NEXT_ARG();
			if (get_time(&rtt_typ, *argv)) {
				fprintf(stderr, "Illegal \"rtt_typ\"\n");
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

	if (rtt_max || rtt_typ) {
		float alpha_f, beta_f;
		SPRINT_BUF(max_rtt_t);
		SPRINT_BUF(typ_rtt_t);
		SPRINT_BUF(tupdate_t);
		SPRINT_BUF(target_t);

		if (!rtt_typ)
			rtt_typ = fmax(rtt_max / RTT_TYP_TO_MAX, 1U);
		else if (!rtt_max)
			rtt_max = rtt_typ * RTT_TYP_TO_MAX;
		else if (rtt_typ > rtt_max) {
			fprintf(stderr, "typical_rtt must be >= max_rtt!\n");
			return -1;
		}
		if (alpha != DEFAULT_ALPHA_BETA || beta != DEFAULT_ALPHA_BETA ||
		    tupdate || target)
			fprintf(stderr, "rtt_max is specified, ignoring values "
				"specified for alpha/beta/tupdate/target\n");
		target = rtt_typ;
		tupdate = fmax(min(rtt_typ, rtt_max / 3), 1U);
		alpha_f = (double)tupdate / ((double)rtt_max * rtt_max)
			* TIME_UNITS_PER_SEC * 0.1f;
		beta_f = 0.3f / (float)rtt_max * TIME_UNITS_PER_SEC;
		if (beta_f > ALPHA_BETA_MAX) {
			fprintf(stderr, "max_rtt=%s is too low and cause beta "
				"to overflow!\n",
				sprint_time(rtt_max, max_rtt_t));
			return -1;
		}
		if (alpha_f < 1.0f / ALPHA_BETA_SCALE ||
		    beta_f < 1.0f / ALPHA_BETA_SCALE) {
			fprintf(stderr, "max_rtt=%s is too large and will "
				"cause alpha=%f and/or beta=%f to be rounded "
				"down to 0!\n", sprint_time(rtt_max, max_rtt_t),
				alpha_f, beta_f);
			return -1;
		}
		fprintf(stderr, "Auto-configuring parameters using "
			"[max_rtt: %s, typical_rtt: %s]: "
			"target=%s tupdate=%s alpha=%f beta=%f\n",
			sprint_time(rtt_max, max_rtt_t),
			sprint_time(rtt_typ, typ_rtt_t),
			sprint_time(target, target_t),
			sprint_time(tupdate, tupdate_t), alpha_f, beta_f);
		alpha = alpha_f * ALPHA_BETA_SCALE;
		beta = beta * ALPHA_BETA_SCALE;
	}

	tail = addattr_nest(n, 1024, TCA_OPTIONS);
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
	addattr_nest_end(n, tail);
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
