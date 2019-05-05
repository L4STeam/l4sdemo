/*
 * iplink_bridge_slave.c	Bridge slave device support
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Authors:     Jiri Pirko <jiri@resnulli.us>
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_link.h>
#include <linux/if_bridge.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"

static void print_explain(FILE *f)
{
	fprintf(f,
		"Usage: ... bridge_slave [ state STATE ] [ priority PRIO ] [cost COST ]\n"
		"                        [ guard {on | off} ]\n"
		"                        [ hairpin {on | off} ]\n"
		"                        [ fastleave {on | off} ]\n"
		"                        [ root_block {on | off} ]\n"
		"                        [ learning {on | off} ]\n"
		"                        [ flood {on | off} ]\n"
		"                        [ proxy_arp {on | off} ]\n"
		"                        [ proxy_arp_wifi {on | off} ]\n"
		"                        [ mcast_router MULTICAST_ROUTER ]\n"
		"                        [ mcast_fast_leave {on | off} ]\n"
		"                        [ mcast_flood {on | off} ]\n"
	);
}

static void explain(void)
{
	print_explain(stderr);
}

static const char *port_states[] = {
	[BR_STATE_DISABLED] = "disabled",
	[BR_STATE_LISTENING] = "listening",
	[BR_STATE_LEARNING] = "learning",
	[BR_STATE_FORWARDING] = "forwarding",
	[BR_STATE_BLOCKING] = "blocking",
};

static void print_portstate(FILE *f, __u8 state)
{
	if (state <= BR_STATE_BLOCKING)
		fprintf(f, "state %s ", port_states[state]);
	else
		fprintf(f, "state (%d) ", state);
}

static void print_onoff(FILE *f, char *flag, __u8 val)
{
	fprintf(f, "%s %s ", flag, val ? "on" : "off");
}

static void bridge_slave_print_opt(struct link_util *lu, FILE *f,
				   struct rtattr *tb[])
{
	if (!tb)
		return;

	if (tb[IFLA_BRPORT_STATE])
		print_portstate(f, rta_getattr_u8(tb[IFLA_BRPORT_STATE]));

	if (tb[IFLA_BRPORT_PRIORITY])
		fprintf(f, "priority %d ",
			rta_getattr_u16(tb[IFLA_BRPORT_PRIORITY]));

	if (tb[IFLA_BRPORT_COST])
		fprintf(f, "cost %d ",
			rta_getattr_u32(tb[IFLA_BRPORT_COST]));

	if (tb[IFLA_BRPORT_MODE])
		print_onoff(f, "hairpin",
			    rta_getattr_u8(tb[IFLA_BRPORT_MODE]));

	if (tb[IFLA_BRPORT_GUARD])
		print_onoff(f, "guard",
			    rta_getattr_u8(tb[IFLA_BRPORT_GUARD]));

	if (tb[IFLA_BRPORT_PROTECT])
		print_onoff(f, "root_block",
			    rta_getattr_u8(tb[IFLA_BRPORT_PROTECT]));

	if (tb[IFLA_BRPORT_FAST_LEAVE])
		print_onoff(f, "fastleave",
			    rta_getattr_u8(tb[IFLA_BRPORT_FAST_LEAVE]));

	if (tb[IFLA_BRPORT_LEARNING])
		print_onoff(f, "learning",
			rta_getattr_u8(tb[IFLA_BRPORT_LEARNING]));

	if (tb[IFLA_BRPORT_UNICAST_FLOOD])
		print_onoff(f, "flood",
			rta_getattr_u8(tb[IFLA_BRPORT_UNICAST_FLOOD]));

	if (tb[IFLA_BRPORT_ID])
		fprintf(f, "port_id 0x%x ",
			rta_getattr_u16(tb[IFLA_BRPORT_ID]));

	if (tb[IFLA_BRPORT_NO])
		fprintf(f, "port_no 0x%x ",
			rta_getattr_u16(tb[IFLA_BRPORT_NO]));

	if (tb[IFLA_BRPORT_DESIGNATED_PORT])
		fprintf(f, "designated_port %u ",
			rta_getattr_u16(tb[IFLA_BRPORT_DESIGNATED_PORT]));

	if (tb[IFLA_BRPORT_DESIGNATED_COST])
		fprintf(f, "designated_cost %u ",
			rta_getattr_u16(tb[IFLA_BRPORT_DESIGNATED_COST]));

	if (tb[IFLA_BRPORT_BRIDGE_ID]) {
		char bridge_id[32];

		br_dump_bridge_id(RTA_DATA(tb[IFLA_BRPORT_BRIDGE_ID]),
				  bridge_id, sizeof(bridge_id));
		fprintf(f, "designated_bridge %s ", bridge_id);
	}

	if (tb[IFLA_BRPORT_ROOT_ID]) {
		char root_id[32];

		br_dump_bridge_id(RTA_DATA(tb[IFLA_BRPORT_ROOT_ID]),
				  root_id, sizeof(root_id));
		fprintf(f, "designated_root %s ", root_id);
	}

	if (tb[IFLA_BRPORT_HOLD_TIMER]) {
		struct timeval tv;
		__u64 htimer;

		htimer = rta_getattr_u64(tb[IFLA_BRPORT_HOLD_TIMER]);
		__jiffies_to_tv(&tv, htimer);
		fprintf(f, "hold_timer %4i.%.2i ", (int)tv.tv_sec,
			(int)tv.tv_usec/10000);
	}

	if (tb[IFLA_BRPORT_MESSAGE_AGE_TIMER]) {
		struct timeval tv;
		__u64 agetimer;

		agetimer = rta_getattr_u64(tb[IFLA_BRPORT_MESSAGE_AGE_TIMER]);
		__jiffies_to_tv(&tv, agetimer);
		fprintf(f, "message_age_timer %4i.%.2i ", (int)tv.tv_sec,
			(int)tv.tv_usec/10000);
	}

	if (tb[IFLA_BRPORT_FORWARD_DELAY_TIMER]) {
		struct timeval tv;
		__u64 fwdtimer;

		fwdtimer = rta_getattr_u64(tb[IFLA_BRPORT_FORWARD_DELAY_TIMER]);
		__jiffies_to_tv(&tv, fwdtimer);
		fprintf(f, "forward_delay_timer %4i.%.2i ", (int)tv.tv_sec,
			(int)tv.tv_usec/10000);
	}

	if (tb[IFLA_BRPORT_TOPOLOGY_CHANGE_ACK])
		fprintf(f, "topology_change_ack %u ",
			rta_getattr_u8(tb[IFLA_BRPORT_TOPOLOGY_CHANGE_ACK]));

	if (tb[IFLA_BRPORT_CONFIG_PENDING])
		fprintf(f, "config_pending %u ",
			rta_getattr_u8(tb[IFLA_BRPORT_CONFIG_PENDING]));
	if (tb[IFLA_BRPORT_PROXYARP])
		print_onoff(f, "proxy_arp",
			    rta_getattr_u8(tb[IFLA_BRPORT_PROXYARP]));

	if (tb[IFLA_BRPORT_PROXYARP_WIFI])
		print_onoff(f, "proxy_arp_wifi",
			    rta_getattr_u8(tb[IFLA_BRPORT_PROXYARP_WIFI]));

	if (tb[IFLA_BRPORT_MULTICAST_ROUTER])
		fprintf(f, "mcast_router %u ",
			rta_getattr_u8(tb[IFLA_BRPORT_MULTICAST_ROUTER]));

	if (tb[IFLA_BRPORT_FAST_LEAVE])
		print_onoff(f, "mcast_fast_leave",
			    rta_getattr_u8(tb[IFLA_BRPORT_FAST_LEAVE]));

	if (tb[IFLA_BRPORT_MCAST_FLOOD])
		print_onoff(f, "mcast_flood",
			rta_getattr_u8(tb[IFLA_BRPORT_MCAST_FLOOD]));
}

static void bridge_slave_parse_on_off(char *arg_name, char *arg_val,
				      struct nlmsghdr *n, int type)
{
	__u8 val;

	if (strcmp(arg_val, "on") == 0)
		val = 1;
	else if (strcmp(arg_val, "off") == 0)
		val = 0;
	else
		invarg("should be \"on\" or \"off\"", arg_name);

	addattr8(n, 1024, type, val);
}

static int bridge_slave_parse_opt(struct link_util *lu, int argc, char **argv,
				  struct nlmsghdr *n)
{
	__u8 state;
	__u16 priority;
	__u32 cost;

	while (argc > 0) {
		if (matches(*argv, "state") == 0) {
			NEXT_ARG();
			if (get_u8(&state, *argv, 0))
				invarg("state is invalid", *argv);
			addattr8(n, 1024, IFLA_BRPORT_STATE, state);
		} else if (matches(*argv, "priority") == 0) {
			NEXT_ARG();
			if (get_u16(&priority, *argv, 0))
				invarg("priority is invalid", *argv);
			addattr16(n, 1024, IFLA_BRPORT_PRIORITY, priority);
		} else if (matches(*argv, "cost") == 0) {
			NEXT_ARG();
			if (get_u32(&cost, *argv, 0))
				invarg("cost is invalid", *argv);
			addattr32(n, 1024, IFLA_BRPORT_COST, cost);
		} else if (matches(*argv, "hairpin") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("hairpin", *argv, n,
						  IFLA_BRPORT_MODE);
		} else if (matches(*argv, "guard") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("guard", *argv, n,
						  IFLA_BRPORT_GUARD);
		} else if (matches(*argv, "root_block") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("root_block", *argv, n,
						  IFLA_BRPORT_PROTECT);
		} else if (matches(*argv, "fastleave") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("fastleave", *argv, n,
						  IFLA_BRPORT_FAST_LEAVE);
		} else if (matches(*argv, "learning") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("learning", *argv, n,
						  IFLA_BRPORT_LEARNING);
		} else if (matches(*argv, "flood") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("flood", *argv, n,
						  IFLA_BRPORT_UNICAST_FLOOD);
		} else if (matches(*argv, "mcast_flood") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("mcast_flood", *argv, n,
						  IFLA_BRPORT_MCAST_FLOOD);
		} else if (matches(*argv, "proxy_arp") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("proxy_arp", *argv, n,
						  IFLA_BRPORT_PROXYARP);
		} else if (matches(*argv, "proxy_arp_wifi") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("proxy_arp_wifi", *argv, n,
						  IFLA_BRPORT_PROXYARP_WIFI);
		} else if (matches(*argv, "mcast_router") == 0) {
			__u8 mcast_router;

			NEXT_ARG();
			if (get_u8(&mcast_router, *argv, 0))
				invarg("invalid mcast_router", *argv);
			addattr8(n, 1024, IFLA_BRPORT_MULTICAST_ROUTER,
				 mcast_router);
		} else if (matches(*argv, "mcast_fast_leave") == 0) {
			NEXT_ARG();
			bridge_slave_parse_on_off("mcast_fast_leave", *argv, n,
						  IFLA_BRPORT_FAST_LEAVE);
		} else if (matches(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "bridge_slave: unknown option \"%s\"?\n",
				*argv);
			explain();
			return -1;
		}
		argc--, argv++;
	}

	return 0;
}

static void bridge_slave_print_help(struct link_util *lu, int argc, char **argv,
		FILE *f)
{
	print_explain(f);
}

struct link_util bridge_slave_link_util = {
	.id		= "bridge_slave",
	.maxattr	= IFLA_BRPORT_MAX,
	.print_opt	= bridge_slave_print_opt,
	.parse_opt	= bridge_slave_parse_opt,
	.print_help     = bridge_slave_print_help,
};
