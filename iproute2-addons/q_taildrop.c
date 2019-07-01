/* derived from q_fifo.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "utils.h"
#include "tc_util.h"

static void explain(void)
{
	fprintf(stderr, "Usage: ... taildrop [ limit BYTES ]\n");
}

/* iproute2 v4.15 changed the API in commit b317557f5854bb8 / tag v4.15
 * Conveniently, the CBS scheduler got introduced in Linux in commit
 * 0f7787b4133fb / tag v4.15.
 * Checking for the presence of *CBS*-related defines in pkt_sched.h is thus
 * a sign that we are on iproute2 >= 4.15
 */
static int taildrop_parse_opt(struct qdisc_util *qu, int argc, char **argv,
#ifdef TCA_CBS_MAX
			 struct nlmsghdr *n, const char* dev)
#else
			 struct nlmsghdr *n)
#endif
{
	int ok = 0;
	struct tc_fifo_qopt opt = {};

	while (argc > 0) {
		if (strcmp(*argv, "limit") == 0) {
			NEXT_ARG();
			if (get_size(&opt.limit, *argv)) {
				fprintf(stderr, "%s: Illegal value for \"limit\": \"%s\"\n", qu->id, *argv);
				return -1;
			}
			ok++;
		} else if (strcmp(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "%s: unknown parameter \"%s\"\n", qu->id, *argv);
			explain();
			return -1;
		}
		argc--; argv++;
	}

	if (ok)
		addattr_l(n, 1024, TCA_OPTIONS, &opt, sizeof(opt));
	return 0;
}

static int taildrop_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct tc_fifo_qopt *qopt;

	if (opt == NULL)
		return 0;

	if (RTA_PAYLOAD(opt)  < sizeof(*qopt))
		return -1;
	qopt = RTA_DATA(opt);
	SPRINT_BUF(b1);
	fprintf(f, "limit %s", sprint_size(qopt->limit, b1));
	return 0;
}


struct qdisc_util taildrop_qdisc_util = {
	.id = "taildrop",
	.parse_qopt = taildrop_parse_opt,
	.print_qopt = taildrop_print_opt,
};
