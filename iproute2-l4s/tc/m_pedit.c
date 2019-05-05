/*
 * m_pedit.c		generic packet editor actions module
 *
 *		This program is free software; you can distribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:  J Hadi Salim (hadi@cyberus.ca)
 *
 * TODO:
 *	1) Big endian broken in some spots
 *	2) A lot of this stuff was added on the fly; get a big double-double
 *	and clean it up at some point.
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
#include <dlfcn.h>
#include "utils.h"
#include "tc_util.h"
#include "m_pedit.h"

static struct m_pedit_util *pedit_list;
static int pedit_debug;

static void explain(void)
{
	fprintf(stderr, "Usage: ... pedit munge <MUNGE> [CONTROL]\n");
	fprintf(stderr,
		"Where: MUNGE := <RAW>|<LAYERED>\n"
		"\t<RAW>:= <OFFSETC>[ATC]<CMD>\n \t\tOFFSETC:= offset <offval> <u8|u16|u32>\n"
		"\t\tATC:= at <atval> offmask <maskval> shift <shiftval>\n"
		"\t\tNOTE: offval is byte offset, must be multiple of 4\n"
		"\t\tNOTE: maskval is a 32 bit hex number\n \t\tNOTE: shiftval is a is a shift value\n"
		"\t\tCMD:= clear | invert | set <setval>| retain\n"
		"\t<LAYERED>:= ip <ipdata> | ip6 <ip6data>\n"
		" \t\t| udp <udpdata> | tcp <tcpdata> | icmp <icmpdata>\n"
		"\tCONTROL:= reclassify | pipe | drop | continue | pass\n"
		"For Example usage look at the examples directory\n");

}

static void usage(void)
{
	explain();
	exit(-1);
}

static int pedit_parse_nopopt(int *argc_p, char ***argv_p,
			      struct tc_pedit_sel *sel,
			      struct tc_pedit_key *tkey)
{
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc) {
		fprintf(stderr,
			"Unknown action  hence option \"%s\" is unparsable\n",
			*argv);
		return -1;
	}

	return 0;

}

static struct m_pedit_util *get_pedit_kind(const char *str)
{
	static void *pBODY;
	void *dlh;
	char buf[256];
	struct m_pedit_util *p;

	for (p = pedit_list; p; p = p->next) {
		if (strcmp(p->id, str) == 0)
			return p;
	}

	snprintf(buf, sizeof(buf), "p_%s.so", str);
	dlh = dlopen(buf, RTLD_LAZY);
	if (dlh == NULL) {
		dlh = pBODY;
		if (dlh == NULL) {
			dlh = pBODY = dlopen(NULL, RTLD_LAZY);
			if (dlh == NULL)
				goto noexist;
		}
	}

	snprintf(buf, sizeof(buf), "p_pedit_%s", str);
	p = dlsym(dlh, buf);
	if (p == NULL)
		goto noexist;

reg:
	p->next = pedit_list;
	pedit_list = p;
	return p;

noexist:
	p = calloc(1, sizeof(*p));
	if (p) {
		strncpy(p->id, str, sizeof(p->id) - 1);
		p->parse_peopt = pedit_parse_nopopt;
		goto reg;
	}
	return p;
}

int pack_key(struct tc_pedit_sel *sel, struct tc_pedit_key *tkey)
{
	int hwm = sel->nkeys;

	if (hwm >= MAX_OFFS)
		return -1;

	if (tkey->off % 4) {
		fprintf(stderr, "offsets MUST be in 32 bit boundaries\n");
		return -1;
	}

	sel->keys[hwm].val = tkey->val;
	sel->keys[hwm].mask = tkey->mask;
	sel->keys[hwm].off = tkey->off;
	sel->keys[hwm].at = tkey->at;
	sel->keys[hwm].offmask = tkey->offmask;
	sel->keys[hwm].shift = tkey->shift;
	sel->nkeys++;
	return 0;
}

int pack_key32(__u32 retain, struct tc_pedit_sel *sel,
	       struct tc_pedit_key *tkey)
{
	if (tkey->off > (tkey->off & ~3)) {
		fprintf(stderr,
			"pack_key32: 32 bit offsets must begin in 32bit boundaries\n");
		return -1;
	}

	tkey->val = htonl(tkey->val & retain);
	tkey->mask = htonl(tkey->mask | ~retain);
	return pack_key(sel, tkey);
}

int pack_key16(__u32 retain, struct tc_pedit_sel *sel,
	       struct tc_pedit_key *tkey)
{
	int ind, stride;
	__u32 m[4] = { 0x0000FFFF, 0xFF0000FF, 0xFFFF0000 };

	if (tkey->val > 0xFFFF || tkey->mask > 0xFFFF) {
		fprintf(stderr, "pack_key16 bad value\n");
		return -1;
	}

	ind = tkey->off & 3;

	if (ind == 3) {
		fprintf(stderr, "pack_key16 bad index value %d\n", ind);
		return -1;
	}

	stride = 8 * (2 - ind);
	tkey->val = htonl((tkey->val & retain) << stride);
	tkey->mask = htonl(((tkey->mask | ~retain) << stride) | m[ind]);

	tkey->off &= ~3;

	if (pedit_debug)
		printf("pack_key16: Final val %08x mask %08x\n",
		       tkey->val, tkey->mask);
	return pack_key(sel, tkey);

}

int pack_key8(__u32 retain, struct tc_pedit_sel *sel, struct tc_pedit_key *tkey)
{
	int ind, stride;
	__u32 m[4] = { 0x00FFFFFF, 0xFF00FFFF, 0xFFFF00FF, 0xFFFFFF00 };

	if (tkey->val > 0xFF || tkey->mask > 0xFF) {
		fprintf(stderr, "pack_key8 bad value (val %x mask %x\n",
			tkey->val, tkey->mask);
		return -1;
	}

	ind = tkey->off & 3;

	stride = 8 * (3 - ind);
	tkey->val = htonl((tkey->val & retain) << stride);
	tkey->mask = htonl(((tkey->mask | ~retain) << stride) | m[ind]);

	tkey->off &= ~3;

	if (pedit_debug)
		printf("pack_key8: Final word off %d  val %08x mask %08x\n",
		       tkey->off, tkey->val, tkey->mask);
	return pack_key(sel, tkey);
}

int parse_val(int *argc_p, char ***argv_p, __u32 *val, int type)
{
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc <= 0)
		return -1;

	if (type == TINT)
		return get_integer((int *)val, *argv, 0);

	if (type == TU32)
		return get_u32(val, *argv, 0);

	if (type == TIPV4) {
		inet_prefix addr;

		if (get_prefix_1(&addr, *argv, AF_INET))
			return -1;

		*val = addr.data[0];
		return 0;
	}

	if (type == TIPV6)
		return -1; /* not implemented yet */

	return -1;
}

int parse_cmd(int *argc_p, char ***argv_p, __u32 len, int type, __u32 retain,
	      struct tc_pedit_sel *sel, struct tc_pedit_key *tkey)
{
	__u32 mask = 0, val = 0;
	__u32 o = 0xFF;
	int res = -1;
	int argc = *argc_p;
	char **argv = *argv_p;

	if (argc <= 0)
		return -1;

	if (pedit_debug)
		printf("parse_cmd argc %d %s offset %d length %d\n",
		       argc, *argv, tkey->off, len);

	if (len == 2)
		o = 0xFFFF;
	if (len == 4)
		o = 0xFFFFFFFF;

	if (matches(*argv, "invert") == 0) {
		val = mask = o;
	} else if (matches(*argv, "set") == 0) {
		NEXT_ARG();
		if (parse_val(&argc, &argv, &val, type))
			return -1;
	} else if (matches(*argv, "preserve") == 0) {
		retain = 0;
	} else {
		if (matches(*argv, "clear") != 0)
			return -1;
	}

	argc--;
	argv++;

	if (argc && matches(*argv, "retain") == 0) {
		NEXT_ARG();
		if (parse_val(&argc, &argv, &retain, TU32))
			return -1;
		argc--;
		argv++;
	}

	tkey->val = val;
	tkey->mask = mask;

	if (type == TIPV4)
		tkey->val = ntohl(tkey->val);

	if (len == 1) {
		res = pack_key8(retain, sel, tkey);
		goto done;
	}
	if (len == 2) {
		res = pack_key16(retain, sel, tkey);
		goto done;
	}
	if (len == 4) {
		res = pack_key32(retain, sel, tkey);
		goto done;
	}

	return -1;
done:
	if (pedit_debug)
		printf("parse_cmd done argc %d %s offset %d length %d\n",
		       argc, *argv, tkey->off, len);
	*argc_p = argc;
	*argv_p = argv;
	return res;

}

int parse_offset(int *argc_p, char ***argv_p, struct tc_pedit_sel *sel,
		 struct tc_pedit_key *tkey)
{
	int off;
	__u32 len, retain;
	int argc = *argc_p;
	char **argv = *argv_p;
	int res = -1;

	if (argc <= 0)
		return -1;

	if (get_integer(&off, *argv, 0))
		return -1;
	tkey->off = off;

	argc--;
	argv++;

	if (argc <= 0)
		return -1;

	if (matches(*argv, "u32") == 0) {
		len = 4;
		retain = 0xFFFFFFFF;
		goto done;
	}
	if (matches(*argv, "u16") == 0) {
		len = 2;
		retain = 0xffff;
		goto done;
	}
	if (matches(*argv, "u8") == 0) {
		len = 1;
		retain = 0xff;
		goto done;
	}

	return -1;

done:

	NEXT_ARG();

	/* [at <someval> offmask <maskval> shift <shiftval>] */
	if (matches(*argv, "at") == 0) {

		__u32 atv = 0, offmask = 0x0, shift = 0;

		NEXT_ARG();
		if (get_u32(&atv, *argv, 0))
			return -1;
		tkey->at = atv;

		NEXT_ARG();

		if (get_u32(&offmask, *argv, 16))
			return -1;
		tkey->offmask = offmask;

		NEXT_ARG();

		if (get_u32(&shift, *argv, 0))
			return -1;
		tkey->shift = shift;

		NEXT_ARG();
	}

	res = parse_cmd(&argc, &argv, len, TU32, retain, sel, tkey);

	*argc_p = argc;
	*argv_p = argv;
	return res;
}

static int parse_munge(int *argc_p, char ***argv_p, struct tc_pedit_sel *sel)
{
	struct tc_pedit_key tkey = {};
	int argc = *argc_p;
	char **argv = *argv_p;
	int res = -1;

	if (argc <= 0)
		return -1;

	if (matches(*argv, "offset") == 0) {
		NEXT_ARG();
		res = parse_offset(&argc, &argv, sel, &tkey);
		goto done;
	} else {
		char k[16];
		struct m_pedit_util *p = NULL;

		strncpy(k, *argv, sizeof(k) - 1);

		if (argc > 0) {
			p = get_pedit_kind(k);
			if (p == NULL)
				goto bad_val;
			NEXT_ARG();
			res = p->parse_peopt(&argc, &argv, sel, &tkey);
			if (res < 0) {
				fprintf(stderr, "bad pedit parsing\n");
				goto bad_val;
			}
			goto done;
		}
	}

bad_val:
	return -1;

done:

	*argc_p = argc;
	*argv_p = argv;
	return res;
}

int parse_pedit(struct action_util *a, int *argc_p, char ***argv_p, int tca_id,
		struct nlmsghdr *n)
{
	struct {
		struct tc_pedit_sel sel;
		struct tc_pedit_key keys[MAX_OFFS];
	} sel = {};

	int argc = *argc_p;
	char **argv = *argv_p;
	int ok = 0, iok = 0;
	struct rtattr *tail;

	while (argc > 0) {
		if (pedit_debug > 1)
			fprintf(stderr, "while pedit (%d:%s)\n", argc, *argv);
		if (matches(*argv, "pedit") == 0) {
			NEXT_ARG();
			ok++;
			continue;
		} else if (matches(*argv, "help") == 0) {
			usage();
		} else if (matches(*argv, "munge") == 0) {
			if (!ok) {
				fprintf(stderr, "Bad pedit construct (%s)\n",
					*argv);
				explain();
				return -1;
			}
			NEXT_ARG();
			if (parse_munge(&argc, &argv, &sel.sel)) {
				fprintf(stderr, "Bad pedit construct (%s)\n",
					*argv);
				explain();
				return -1;
			}
			ok++;
		} else {
			break;
		}

	}

	if (!ok) {
		explain();
		return -1;
	}

	if (argc && !action_a2n(*argv, &sel.sel.action, false))
		NEXT_ARG();

	if (argc) {
		if (matches(*argv, "index") == 0) {
			NEXT_ARG();
			if (get_u32(&sel.sel.index, *argv, 10)) {
				fprintf(stderr, "Pedit: Illegal \"index\"\n");
				return -1;
			}
			argc--;
			argv++;
			iok++;
		}
	}

	tail = NLMSG_TAIL(n);
	addattr_l(n, MAX_MSG, tca_id, NULL, 0);
	addattr_l(n, MAX_MSG, TCA_PEDIT_PARMS, &sel,
		  sizeof(sel.sel) +
		  sel.sel.nkeys * sizeof(struct tc_pedit_key));
	tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;

	*argc_p = argc;
	*argv_p = argv;
	return 0;
}

int print_pedit(struct action_util *au, FILE *f, struct rtattr *arg)
{
	struct tc_pedit_sel *sel;
	struct rtattr *tb[TCA_PEDIT_MAX + 1];

	if (arg == NULL)
		return -1;

	parse_rtattr_nested(tb, TCA_PEDIT_MAX, arg);

	if (tb[TCA_PEDIT_PARMS] == NULL) {
		fprintf(f, "[NULL pedit parameters]");
		return -1;
	}
	sel = RTA_DATA(tb[TCA_PEDIT_PARMS]);

	fprintf(f, " pedit action %s keys %d\n ",
		action_n2a(sel->action), sel->nkeys);
	fprintf(f, "\t index %u ref %d bind %d", sel->index, sel->refcnt,
		sel->bindcnt);

	if (show_stats) {
		if (tb[TCA_PEDIT_TM]) {
			struct tcf_t *tm = RTA_DATA(tb[TCA_PEDIT_TM]);

			print_tm(f, tm);
		}
	}
	if (sel->nkeys) {
		int i;
		struct tc_pedit_key *key = sel->keys;

		for (i = 0; i < sel->nkeys; i++, key++) {
			fprintf(f, "\n\t key #%d", i);
			fprintf(f, "  at %d: val %08x mask %08x",
				(unsigned int)key->off,
				(unsigned int)ntohl(key->val),
				(unsigned int)ntohl(key->mask));
		}
	} else {
		fprintf(f, "\npedit %x keys %d is not LEGIT", sel->index,
			sel->nkeys);
	}

	fprintf(f, "\n ");
	return 0;
}

int pedit_print_xstats(struct action_util *au, FILE *f, struct rtattr *xstats)
{
	return 0;
}

struct action_util pedit_action_util = {
	.id = "pedit",
	.parse_aopt = parse_pedit,
	.print_aopt = print_pedit,
};
