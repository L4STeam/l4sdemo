/*
 * dualq.c		Dualqueue qdisc
 *
 * ...
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

#include "utils.h"
#include "tc_util.h"

static void explain(void)
{
	fprintf(stderr, "Usage: ... dualq l_thresh[_us] <packets [or us]> offset <ms> l_slope <bit shifts> c_slope <bit shifts> l_smooth <bit shifts> c_smooth <bit shifts> l_power <number> c_power <number> l_shift <ms> l_speed <bit shift>\n");
}

static int dualq_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			    struct nlmsghdr *n)
{
	printf("Dualq parse opt\n");
	struct tc_dualq_qopt opt;
	opt.ecn_thresh = 5;  // packet sized queue size when LL packets get marked
	opt.et_packets_us = 0;  // ecn threshold in packets (0), us (1)
	opt.offset = 0;      // offset of (curvy) RED (in ms); up to now always 0
	opt.dcslope = 6;     // slope of LL RED (in bit shifts)
	opt.renoslope = 5;   // slope of C curvy-RED (in bit shifts)
	opt.k_ll = 0;        // smoothing for LL RED (in bit shifts)
	opt.k_c = 5;         // smoothing for C curvy-RED (in bit shifts)
	opt.p_ll = 1;        // power of LL traffic (1 for RED)
	opt.p_c = 2;         // power of C traffic (2 for curvy-RED)
	opt.tshift = 0xFFFF; // LL FIFO time shift (in ms)
	opt.tspeed = 0;      // LL FIFO time speed (in bit shifts)

        while (argc > 0) {
                if ((strcmp(*argv, "l_thresh") == 0)||(strcmp(*argv, "ecn_thresh") == 0)) {
                        NEXT_ARG();
                        opt.et_packets_us = 0;
                        if (get_u32(&opt.ecn_thresh, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_thresh\"\n");
                                return -1;
                        }
                } else if (strcmp(*argv, "l_thresh_us") == 0) {
                        NEXT_ARG();
                        opt.et_packets_us = 1;
                        if (get_u32(&opt.ecn_thresh, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_thresh_us\"\n");
                                return -1;
                        }
                } else if (strcmp(*argv, "offset") == 0) {
                        NEXT_ARG();
                        if (get_u16(&opt.offset, *argv, 0)) {
                                fprintf(stderr, "Illegal \"offset\"\n");
                                return -1;
                        }
                } else if ((strcmp(*argv, "l_slope") == 0)||(strcmp(*argv, "dcslope") == 0)) {
                        NEXT_ARG();
                        if (get_u16(&opt.dcslope, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_slope\"\n");
                                return -1;
                        }
                } else if ((strcmp(*argv, "c_slope") == 0)||(strcmp(*argv, "renoslope") == 0)) {
                        NEXT_ARG();
                        if (get_u16(&opt.renoslope, *argv, 0)) {
                                fprintf(stderr, "Illegal \"c_slope\"\n");
                                return -1;
                        }
                } else if ((strcmp(*argv, "l_smooth") == 0)||(strcmp(*argv, "k_ll") == 0)) {
                        NEXT_ARG();
                        if (get_u16(&opt.k_ll, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_smooth\"\n");
                                return -1;
                        }
                } else if ((strcmp(*argv, "c_smooth") == 0)||(strcmp(*argv, "k_c") == 0)) {
                        NEXT_ARG();
                        if (get_u16(&opt.k_c, *argv, 0)) {
                                fprintf(stderr, "Illegal \"c_smooth\"\n");
                                return -1;
                        }
                } else if ((strcmp(*argv, "l_power") == 0)||(strcmp(*argv, "p_ll") == 0)) {
                        NEXT_ARG();
                        if (get_u16(&opt.p_ll, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_power\"\n");
                                return -1;
                        }
                } else if ((strcmp(*argv, "c_power") == 0)||(strcmp(*argv, "p_c") == 0)) {
                        NEXT_ARG();
                        if (get_u16(&opt.p_c, *argv, 0)) {
                                fprintf(stderr, "Illegal \"c_power\"\n");
                                return -1;
                        }
                } else if (strcmp(*argv, "l_shift") == 0) {
                        NEXT_ARG();
                        if (get_u16(&opt.tshift, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_shift\"\n");
                                return -1;
                        }
                } else if (strcmp(*argv, "l_speed") == 0) {
                        NEXT_ARG();
                        if (get_u16(&opt.tspeed, *argv, 0)) {
                                fprintf(stderr, "Illegal \"l_speed\"\n");
                                return -1;
                        }
                } else {
                        explain();
                        return -1;
                }
                argc--;
                argv++;
        }

fprintf(stderr, "Dualq options: l_thresh %u%s, offset %u, l_slope %u, c_slope %u, l_smooth %u, c_smooth %u, l_power %u, c_power %u, l_shift %u, l_speed %u\n", opt.ecn_thresh, opt.et_packets_us ? "us" : "p", opt.offset, opt.dcslope, opt.renoslope, opt.k_ll, opt.k_c, opt.p_ll, opt.p_c, opt.tshift, opt.tspeed);
        fprintf(stderr, "Size of options: %lu\n", sizeof(opt));

	addattr_l(n, 1024, TCA_OPTIONS, &opt, sizeof(opt));
	return 0;
}

static int dualq_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct tc_dualq_qopt *qopt;

	if (opt == NULL)
		return 0;
	if (RTA_PAYLOAD(opt) < sizeof(*qopt))
		return 0;

	qopt = RTA_DATA(opt);

	//fprintf(f, "bands %u/%u ", qopt->bands, qopt->max_bands);
    fprintf(f, "l_thresh %u%s, offset %u, l_slope %u, c_slope %u, l_smooth %u, c_smooth %u, l_power %u, c_power %u, l_shift %u, l_speed %u\n", qopt->ecn_thresh, qopt->et_packets_us ? "us" : "p", qopt->offset, qopt->dcslope, qopt->renoslope, qopt->k_ll, qopt->k_c, qopt->p_ll, qopt->p_c, qopt->tshift, qopt->tspeed);

	return 0;
}

struct qdisc_util dualq_qdisc_util = {
	.id	 	= "dualq",
	.parse_qopt	= dualq_parse_opt,
	.print_qopt	= dualq_print_opt,
};
