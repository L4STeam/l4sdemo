/*
 * devlink.c	Devlink tool
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Authors:     Jiri Pirko <jiri@mellanox.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <linux/genetlink.h>
#include <linux/devlink.h>
#include <libmnl/libmnl.h>

#include "SNAPSHOT.h"
#include "list.h"
#include "mnlg.h"
#include "json_writer.h"

#define ESWITCH_MODE_LEGACY "legacy"
#define ESWITCH_MODE_SWITCHDEV "switchdev"
#define ESWITCH_INLINE_MODE_NONE "none"
#define ESWITCH_INLINE_MODE_LINK "link"
#define ESWITCH_INLINE_MODE_NETWORK "network"
#define ESWITCH_INLINE_MODE_TRANSPORT "transport"

#define pr_err(args...) fprintf(stderr, ##args)
#define pr_out(args...) fprintf(stdout, ##args)
#define pr_out_sp(num, args...)					\
	do {							\
		int ret = fprintf(stdout, ##args);		\
		if (ret < num)					\
			fprintf(stdout, "%*s", num - ret, "");	\
	} while (0)

static int _mnlg_socket_recv_run(struct mnlg_socket *nlg,
				 mnl_cb_t data_cb, void *data)
{
	int err;

	err = mnlg_socket_recv_run(nlg, data_cb, data);
	if (err < 0) {
		pr_err("devlink answers: %s\n", strerror(errno));
		return -errno;
	}
	return 0;
}

static int _mnlg_socket_sndrcv(struct mnlg_socket *nlg,
			       const struct nlmsghdr *nlh,
			       mnl_cb_t data_cb, void *data)
{
	int err;

	err = mnlg_socket_send(nlg, nlh);
	if (err < 0) {
		pr_err("Failed to call mnlg_socket_send\n");
		return -errno;
	}
	return _mnlg_socket_recv_run(nlg, data_cb, data);
}

static int _mnlg_socket_group_add(struct mnlg_socket *nlg,
				  const char *group_name)
{
	int err;

	err = mnlg_socket_group_add(nlg, group_name);
	if (err < 0) {
		pr_err("Failed to call mnlg_socket_group_add\n");
		return -errno;
	}
	return 0;
}

struct ifname_map {
	struct list_head list;
	char *bus_name;
	char *dev_name;
	uint32_t port_index;
	char *ifname;
};

static struct ifname_map *ifname_map_alloc(const char *bus_name,
					   const char *dev_name,
					   uint32_t port_index,
					   const char *ifname)
{
	struct ifname_map *ifname_map;

	ifname_map = calloc(1, sizeof(*ifname_map));
	if (!ifname_map)
		return NULL;
	ifname_map->bus_name = strdup(bus_name);
	ifname_map->dev_name = strdup(dev_name);
	ifname_map->port_index = port_index;
	ifname_map->ifname = strdup(ifname);
	if (!ifname_map->bus_name || !ifname_map->dev_name ||
	    !ifname_map->ifname) {
		free(ifname_map->ifname);
		free(ifname_map->dev_name);
		free(ifname_map->bus_name);
		free(ifname_map);
		return NULL;
	}
	return ifname_map;
}

static void ifname_map_free(struct ifname_map *ifname_map)
{
	free(ifname_map->ifname);
	free(ifname_map->dev_name);
	free(ifname_map->bus_name);
	free(ifname_map);
}

#define BIT(nr)                 (1UL << (nr))
#define DL_OPT_HANDLE		BIT(0)
#define DL_OPT_HANDLEP		BIT(1)
#define DL_OPT_PORT_TYPE	BIT(2)
#define DL_OPT_PORT_COUNT	BIT(3)
#define DL_OPT_SB		BIT(4)
#define DL_OPT_SB_POOL		BIT(5)
#define DL_OPT_SB_SIZE		BIT(6)
#define DL_OPT_SB_TYPE		BIT(7)
#define DL_OPT_SB_THTYPE	BIT(8)
#define DL_OPT_SB_TH		BIT(9)
#define DL_OPT_SB_TC		BIT(10)
#define DL_OPT_ESWITCH_MODE	BIT(11)
#define DL_OPT_ESWITCH_INLINE_MODE	BIT(12)

struct dl_opts {
	uint32_t present; /* flags of present items */
	char *bus_name;
	char *dev_name;
	uint32_t port_index;
	enum devlink_port_type port_type;
	uint32_t port_count;
	uint32_t sb_index;
	uint16_t sb_pool_index;
	uint32_t sb_pool_size;
	enum devlink_sb_pool_type sb_pool_type;
	enum devlink_sb_threshold_type sb_pool_thtype;
	uint32_t sb_threshold;
	uint16_t sb_tc_index;
	enum devlink_eswitch_mode eswitch_mode;
	enum devlink_eswitch_inline_mode eswitch_inline_mode;
};

struct dl {
	struct mnlg_socket *nlg;
	struct list_head ifname_map_list;
	int argc;
	char **argv;
	bool no_nice_names;
	struct dl_opts opts;
	json_writer_t *jw;
	bool json_output;
	bool pretty_output;
	struct {
		bool present;
		char *bus_name;
		char *dev_name;
		uint32_t port_index;
	} arr_last;
};

static int dl_argc(struct dl *dl)
{
	return dl->argc;
}

static char *dl_argv(struct dl *dl)
{
	if (dl_argc(dl) == 0)
		return NULL;
	return *dl->argv;
}

static void dl_arg_inc(struct dl *dl)
{
	if (dl_argc(dl) == 0)
		return;
	dl->argc--;
	dl->argv++;
}

static char *dl_argv_next(struct dl *dl)
{
	char *ret;

	if (dl_argc(dl) == 0)
		return NULL;

	ret = *dl->argv;
	dl_arg_inc(dl);
	return ret;
}

static char *dl_argv_index(struct dl *dl, unsigned int index)
{
	if (index >= dl_argc(dl))
		return NULL;
	return dl->argv[index];
}

static int strcmpx(const char *str1, const char *str2)
{
	if (strlen(str1) > strlen(str2))
		return -1;
	return strncmp(str1, str2, strlen(str1));
}

static bool dl_argv_match(struct dl *dl, const char *pattern)
{
	if (dl_argc(dl) == 0)
		return false;
	return strcmpx(dl_argv(dl), pattern) == 0;
}

static bool dl_no_arg(struct dl *dl)
{
	return dl_argc(dl) == 0;
}

static int attr_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type;

	type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, DEVLINK_ATTR_MAX) < 0)
		return MNL_CB_ERROR;

	if (type == DEVLINK_ATTR_BUS_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_DEV_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_INDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_TYPE &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_DESIRED_TYPE &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_NETDEV_IFINDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_NETDEV_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_IBDEV_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_INDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_SIZE &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_INGRESS_POOL_COUNT &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_EGRESS_POOL_COUNT &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_INGRESS_TC_COUNT &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_EGRESS_TC_COUNT &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_POOL_INDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_POOL_TYPE &&
	    mnl_attr_validate(attr, MNL_TYPE_U8) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_POOL_SIZE &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_POOL_THRESHOLD_TYPE &&
	    mnl_attr_validate(attr, MNL_TYPE_U8) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_THRESHOLD &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_TC_INDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_OCC_CUR &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_SB_OCC_MAX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_ESWITCH_MODE &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_ESWITCH_INLINE_MODE &&
	    mnl_attr_validate(attr, MNL_TYPE_U8) < 0)
		return MNL_CB_ERROR;
	tb[type] = attr;
	return MNL_CB_OK;
}

static int ifname_map_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
	struct dl *dl = data;
	struct ifname_map *ifname_map;
	const char *bus_name;
	const char *dev_name;
	uint32_t port_ifindex;
	const char *port_ifname;

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_PORT_INDEX])
		return MNL_CB_ERROR;

	if (!tb[DEVLINK_ATTR_PORT_NETDEV_NAME])
		return MNL_CB_OK;

	bus_name = mnl_attr_get_str(tb[DEVLINK_ATTR_BUS_NAME]);
	dev_name = mnl_attr_get_str(tb[DEVLINK_ATTR_DEV_NAME]);
	port_ifindex = mnl_attr_get_u32(tb[DEVLINK_ATTR_PORT_INDEX]);
	port_ifname = mnl_attr_get_str(tb[DEVLINK_ATTR_PORT_NETDEV_NAME]);
	ifname_map = ifname_map_alloc(bus_name, dev_name,
				      port_ifindex, port_ifname);
	if (!ifname_map)
		return MNL_CB_ERROR;
	list_add(&ifname_map->list, &dl->ifname_map_list);

	return MNL_CB_OK;
}

static void ifname_map_fini(struct dl *dl)
{
	struct ifname_map *ifname_map, *tmp;

	list_for_each_entry_safe(ifname_map, tmp,
				 &dl->ifname_map_list, list) {
		list_del(&ifname_map->list);
		ifname_map_free(ifname_map);
	}
}

static int ifname_map_init(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	INIT_LIST_HEAD(&dl->ifname_map_list);

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_GET,
			       NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);

	err = _mnlg_socket_sndrcv(dl->nlg, nlh, ifname_map_cb, dl);
	if (err) {
		ifname_map_fini(dl);
		return err;
	}
	return 0;
}

static int ifname_map_lookup(struct dl *dl, const char *ifname,
			     char **p_bus_name, char **p_dev_name,
			     uint32_t *p_port_index)
{
	struct ifname_map *ifname_map;

	list_for_each_entry(ifname_map, &dl->ifname_map_list, list) {
		if (strcmp(ifname, ifname_map->ifname) == 0) {
			*p_bus_name = ifname_map->bus_name;
			*p_dev_name = ifname_map->dev_name;
			*p_port_index = ifname_map->port_index;
			return 0;
		}
	}
	return -ENOENT;
}

static int ifname_map_rev_lookup(struct dl *dl, const char *bus_name,
				 const char *dev_name, uint32_t port_index,
				 char **p_ifname)
{
	struct ifname_map *ifname_map;

	list_for_each_entry(ifname_map, &dl->ifname_map_list, list) {
		if (strcmp(bus_name, ifname_map->bus_name) == 0 &&
		    strcmp(dev_name, ifname_map->dev_name) == 0 &&
		    port_index == ifname_map->port_index) {
			*p_ifname = ifname_map->ifname;
			return 0;
		}
	}
	return -ENOENT;
}

static unsigned int strslashcount(char *str)
{
	unsigned int count = 0;
	char *pos = str;

	while ((pos = strchr(pos, '/'))) {
		count++;
		pos++;
	}
	return count;
}

static int strslashrsplit(char *str, char **before, char **after)
{
	char *slash;

	slash = strrchr(str, '/');
	if (!slash)
		return -EINVAL;
	*slash = '\0';
	*before = str;
	*after = slash + 1;
	return 0;
}

static int strtouint32_t(const char *str, uint32_t *p_val)
{
	char *endptr;
	unsigned long int val;

	val = strtoul(str, &endptr, 10);
	if (endptr == str || *endptr != '\0')
		return -EINVAL;
	if (val > UINT_MAX)
		return -ERANGE;
	*p_val = val;
	return 0;
}

static int strtouint16_t(const char *str, uint16_t *p_val)
{
	char *endptr;
	unsigned long int val;

	val = strtoul(str, &endptr, 10);
	if (endptr == str || *endptr != '\0')
		return -EINVAL;
	if (val > USHRT_MAX)
		return -ERANGE;
	*p_val = val;
	return 0;
}

static int __dl_argv_handle(char *str, char **p_bus_name, char **p_dev_name)
{
	strslashrsplit(str, p_bus_name, p_dev_name);
	return 0;
}

static int dl_argv_handle(struct dl *dl, char **p_bus_name, char **p_dev_name)
{
	char *str = dl_argv_next(dl);

	if (!str) {
		pr_err("Devlink identification (\"bus_name/dev_name\") expected\n");
		return -EINVAL;
	}
	if (strslashcount(str) != 1) {
		pr_err("Wrong devlink identification string format.\n");
		pr_err("Expected \"bus_name/dev_name\".\n");
		return -EINVAL;
	}
	return __dl_argv_handle(str, p_bus_name, p_dev_name);
}

static int __dl_argv_handle_port(char *str,
				 char **p_bus_name, char **p_dev_name,
				 uint32_t *p_port_index)
{
	char *handlestr = handlestr;
	char *portstr = portstr;
	int err;

	strslashrsplit(str, &handlestr, &portstr);
	err = strtouint32_t(portstr, p_port_index);
	if (err) {
		pr_err("Port index \"%s\" is not a number or not within range\n",
		       portstr);
		return err;
	}
	strslashrsplit(handlestr, p_bus_name, p_dev_name);
	return 0;
}

static int __dl_argv_handle_port_ifname(struct dl *dl, char *str,
					char **p_bus_name, char **p_dev_name,
					uint32_t *p_port_index)
{
	int err;

	err = ifname_map_lookup(dl, str, p_bus_name, p_dev_name,
				p_port_index);
	if (err) {
		pr_err("Netdevice \"%s\" not found\n", str);
		return err;
	}
	return 0;
}

static int dl_argv_handle_port(struct dl *dl, char **p_bus_name,
			       char **p_dev_name, uint32_t *p_port_index)
{
	char *str = dl_argv_next(dl);
	unsigned int slash_count;

	if (!str) {
		pr_err("Port identification (\"bus_name/dev_name/port_index\" or \"netdev ifname\") expected.\n");
		return -EINVAL;
	}
	slash_count = strslashcount(str);
	switch (slash_count) {
	case 0:
		return __dl_argv_handle_port_ifname(dl, str, p_bus_name,
						    p_dev_name, p_port_index);
	case 2:
		return __dl_argv_handle_port(str, p_bus_name,
					     p_dev_name, p_port_index);
	default:
		pr_err("Wrong port identification string format.\n");
		pr_err("Expected \"bus_name/dev_name/port_index\" or \"netdev_ifname\".\n");
		return -EINVAL;
	}
}

static int dl_argv_handle_both(struct dl *dl, char **p_bus_name,
			       char **p_dev_name, uint32_t *p_port_index,
			       uint32_t *p_handle_bit)
{
	char *str = dl_argv_next(dl);
	unsigned int slash_count;
	int err;

	if (!str) {
		pr_err("One of following identifications expected:\n"
		       "Devlink identification (\"bus_name/dev_name\")\n"
		       "Port identification (\"bus_name/dev_name/port_index\" or \"netdev ifname\")\n");
		return -EINVAL;
	}
	slash_count = strslashcount(str);
	if (slash_count == 1) {
		err = __dl_argv_handle(str, p_bus_name, p_dev_name);
		if (err)
			return err;
		*p_handle_bit = DL_OPT_HANDLE;
	} else if (slash_count == 2) {
		err = __dl_argv_handle_port(str, p_bus_name,
					    p_dev_name, p_port_index);
		if (err)
			return err;
		*p_handle_bit = DL_OPT_HANDLEP;
	} else if (slash_count == 0) {
		err = __dl_argv_handle_port_ifname(dl, str, p_bus_name,
						   p_dev_name, p_port_index);
		if (err)
			return err;
		*p_handle_bit = DL_OPT_HANDLEP;
	} else {
		pr_err("Wrong port identification string format.\n");
		pr_err("Expected \"bus_name/dev_name\" or \"bus_name/dev_name/port_index\" or \"netdev_ifname\".\n");
		return -EINVAL;
	}
	return 0;
}

static int dl_argv_uint32_t(struct dl *dl, uint32_t *p_val)
{
	char *str = dl_argv_next(dl);
	int err;

	if (!str) {
		pr_err("Unsigned number argument expected\n");
		return -EINVAL;
	}

	err = strtouint32_t(str, p_val);
	if (err) {
		pr_err("\"%s\" is not a number or not within range\n", str);
		return err;
	}
	return 0;
}

static int dl_argv_uint16_t(struct dl *dl, uint16_t *p_val)
{
	char *str = dl_argv_next(dl);
	int err;

	if (!str) {
		pr_err("Unsigned number argument expected\n");
		return -EINVAL;
	}

	err = strtouint16_t(str, p_val);
	if (err) {
		pr_err("\"%s\" is not a number or not within range\n", str);
		return err;
	}
	return 0;
}

static int dl_argv_str(struct dl *dl, const char **p_str)
{
	const char *str = dl_argv_next(dl);

	if (!str) {
		pr_err("String parameter expected\n");
		return -EINVAL;
	}
	*p_str = str;
	return 0;
}

static int port_type_get(const char *typestr, enum devlink_port_type *p_type)
{
	if (strcmp(typestr, "auto") == 0) {
		*p_type = DEVLINK_PORT_TYPE_AUTO;
	} else if (strcmp(typestr, "eth") == 0) {
		*p_type = DEVLINK_PORT_TYPE_ETH;
	} else if (strcmp(typestr, "ib") == 0) {
		*p_type = DEVLINK_PORT_TYPE_IB;
	} else {
		pr_err("Unknown port type \"%s\"\n", typestr);
		return -EINVAL;
	}
	return 0;
}

static int pool_type_get(const char *typestr, enum devlink_sb_pool_type *p_type)
{
	if (strcmp(typestr, "ingress") == 0) {
		*p_type = DEVLINK_SB_POOL_TYPE_INGRESS;
	} else if (strcmp(typestr, "egress") == 0) {
		*p_type = DEVLINK_SB_POOL_TYPE_EGRESS;
	} else {
		pr_err("Unknown pool type \"%s\"\n", typestr);
		return -EINVAL;
	}
	return 0;
}

static int threshold_type_get(const char *typestr,
			      enum devlink_sb_threshold_type *p_type)
{
	if (strcmp(typestr, "static") == 0) {
		*p_type = DEVLINK_SB_THRESHOLD_TYPE_STATIC;
	} else if (strcmp(typestr, "dynamic") == 0) {
		*p_type = DEVLINK_SB_THRESHOLD_TYPE_DYNAMIC;
	} else {
		pr_err("Unknown threshold type \"%s\"\n", typestr);
		return -EINVAL;
	}
	return 0;
}

static int eswitch_mode_get(const char *typestr,
			    enum devlink_eswitch_mode *p_mode)
{
	if (strcmp(typestr, ESWITCH_MODE_LEGACY) == 0) {
		*p_mode = DEVLINK_ESWITCH_MODE_LEGACY;
	} else if (strcmp(typestr, ESWITCH_MODE_SWITCHDEV) == 0) {
		*p_mode = DEVLINK_ESWITCH_MODE_SWITCHDEV;
	} else {
		pr_err("Unknown eswitch mode \"%s\"\n", typestr);
		return -EINVAL;
	}
	return 0;
}

static int eswitch_inline_mode_get(const char *typestr,
				   enum devlink_eswitch_inline_mode *p_mode)
{
	if (strcmp(typestr, ESWITCH_INLINE_MODE_NONE) == 0) {
		*p_mode = DEVLINK_ESWITCH_INLINE_MODE_NONE;
	} else if (strcmp(typestr, ESWITCH_INLINE_MODE_LINK) == 0) {
		*p_mode = DEVLINK_ESWITCH_INLINE_MODE_LINK;
	} else if (strcmp(typestr, ESWITCH_INLINE_MODE_NETWORK) == 0) {
		*p_mode = DEVLINK_ESWITCH_INLINE_MODE_NETWORK;
	} else if (strcmp(typestr, ESWITCH_INLINE_MODE_TRANSPORT) == 0) {
		*p_mode = DEVLINK_ESWITCH_INLINE_MODE_TRANSPORT;
	} else {
		pr_err("Unknown eswitch inline mode \"%s\"\n", typestr);
		return -EINVAL;
	}
	return 0;
}

static int dl_argv_parse(struct dl *dl, uint32_t o_required,
			 uint32_t o_optional)
{
	struct dl_opts *opts = &dl->opts;
	uint32_t o_all = o_required | o_optional;
	uint32_t o_found = 0;
	int err;

	if (o_required & DL_OPT_HANDLE && o_required & DL_OPT_HANDLEP) {
		uint32_t handle_bit = handle_bit;

		err = dl_argv_handle_both(dl, &opts->bus_name, &opts->dev_name,
					  &opts->port_index, &handle_bit);
		if (err)
			return err;
		o_found |= handle_bit;
	} else if (o_required & DL_OPT_HANDLE) {
		err = dl_argv_handle(dl, &opts->bus_name, &opts->dev_name);
		if (err)
			return err;
		o_found |= DL_OPT_HANDLE;
	} else if (o_required & DL_OPT_HANDLEP) {
		err = dl_argv_handle_port(dl, &opts->bus_name, &opts->dev_name,
					  &opts->port_index);
		if (err)
			return err;
		o_found |= DL_OPT_HANDLEP;
	}

	while (dl_argc(dl)) {
		if (dl_argv_match(dl, "type") &&
		    (o_all & DL_OPT_PORT_TYPE)) {
			const char *typestr;

			dl_arg_inc(dl);
			err = dl_argv_str(dl, &typestr);
			if (err)
				return err;
			err = port_type_get(typestr, &opts->port_type);
			if (err)
				return err;
			o_found |= DL_OPT_PORT_TYPE;
		} else if (dl_argv_match(dl, "count") &&
			   (o_all & DL_OPT_PORT_COUNT)) {
			dl_arg_inc(dl);
			err = dl_argv_uint32_t(dl, &opts->port_count);
			if (err)
				return err;
			o_found |= DL_OPT_PORT_COUNT;
		} else if (dl_argv_match(dl, "sb") &&
			   (o_all & DL_OPT_SB)) {
			dl_arg_inc(dl);
			err = dl_argv_uint32_t(dl, &opts->sb_index);
			if (err)
				return err;
			o_found |= DL_OPT_SB;
		} else if (dl_argv_match(dl, "pool") &&
			   (o_all & DL_OPT_SB_POOL)) {
			dl_arg_inc(dl);
			err = dl_argv_uint16_t(dl, &opts->sb_pool_index);
			if (err)
				return err;
			o_found |= DL_OPT_SB_POOL;
		} else if (dl_argv_match(dl, "size") &&
			   (o_all & DL_OPT_SB_SIZE)) {
			dl_arg_inc(dl);
			err = dl_argv_uint32_t(dl, &opts->sb_pool_size);
			if (err)
				return err;
			o_found |= DL_OPT_SB_SIZE;
		} else if (dl_argv_match(dl, "type") &&
			   (o_all & DL_OPT_SB_TYPE)) {
			const char *typestr;

			dl_arg_inc(dl);
			err = dl_argv_str(dl, &typestr);
			if (err)
				return err;
			err = pool_type_get(typestr, &opts->sb_pool_type);
			if (err)
				return err;
			o_found |= DL_OPT_SB_TYPE;
		} else if (dl_argv_match(dl, "thtype") &&
			   (o_all & DL_OPT_SB_THTYPE)) {
			const char *typestr;

			dl_arg_inc(dl);
			err = dl_argv_str(dl, &typestr);
			if (err)
				return err;
			err = threshold_type_get(typestr,
						 &opts->sb_pool_thtype);
			if (err)
				return err;
			o_found |= DL_OPT_SB_THTYPE;
		} else if (dl_argv_match(dl, "th") &&
			   (o_all & DL_OPT_SB_TH)) {
			dl_arg_inc(dl);
			err = dl_argv_uint32_t(dl, &opts->sb_threshold);
			if (err)
				return err;
			o_found |= DL_OPT_SB_TH;
		} else if (dl_argv_match(dl, "tc") &&
			   (o_all & DL_OPT_SB_TC)) {
			dl_arg_inc(dl);
			err = dl_argv_uint16_t(dl, &opts->sb_tc_index);
			if (err)
				return err;
			o_found |= DL_OPT_SB_TC;
		} else if (dl_argv_match(dl, "mode") &&
			   (o_all & DL_OPT_ESWITCH_MODE)) {
			const char *typestr;

			dl_arg_inc(dl);
			err = dl_argv_str(dl, &typestr);
			if (err)
				return err;
			err = eswitch_mode_get(typestr, &opts->eswitch_mode);
			if (err)
				return err;
			o_found |= DL_OPT_ESWITCH_MODE;
		} else if (dl_argv_match(dl, "inline-mode") &&
			   (o_all & DL_OPT_ESWITCH_INLINE_MODE)) {
			const char *typestr;

			dl_arg_inc(dl);
			err = dl_argv_str(dl, &typestr);
			if (err)
				return err;
			err = eswitch_inline_mode_get(
				typestr, &opts->eswitch_inline_mode);
			if (err)
				return err;
			o_found |= DL_OPT_ESWITCH_INLINE_MODE;
		} else {
			pr_err("Unknown option \"%s\"\n", dl_argv(dl));
			return -EINVAL;
		}
	}

	opts->present = o_found;

	if ((o_optional & DL_OPT_SB) && !(o_found & DL_OPT_SB)) {
		opts->sb_index = 0;
		opts->present |= DL_OPT_SB;
	}

	if ((o_required & DL_OPT_PORT_TYPE) && !(o_found & DL_OPT_PORT_TYPE)) {
		pr_err("Port type option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_PORT_COUNT) &&
	    !(o_found & DL_OPT_PORT_COUNT)) {
		pr_err("Port split count option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_SB_POOL) && !(o_found & DL_OPT_SB_POOL)) {
		pr_err("Pool index option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_SB_SIZE) && !(o_found & DL_OPT_SB_SIZE)) {
		pr_err("Pool size option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_SB_TYPE) && !(o_found & DL_OPT_SB_TYPE)) {
		pr_err("Pool type option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_SB_THTYPE) && !(o_found & DL_OPT_SB_THTYPE)) {
		pr_err("Pool threshold type option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_SB_TH) && !(o_found & DL_OPT_SB_TH)) {
		pr_err("Threshold option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_SB_TC) && !(o_found & DL_OPT_SB_TC)) {
		pr_err("TC index option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_ESWITCH_MODE) &&
	    !(o_found & DL_OPT_ESWITCH_MODE)) {
		pr_err("E-Switch mode option expected.\n");
		return -EINVAL;
	}

	if ((o_required & DL_OPT_ESWITCH_INLINE_MODE) &&
	    !(o_found & DL_OPT_ESWITCH_INLINE_MODE)) {
		pr_err("E-Switch inline-mode option expected.\n");
		return -EINVAL;
	}

	return 0;
}

static void dl_opts_put(struct nlmsghdr *nlh, struct dl *dl)
{
	struct dl_opts *opts = &dl->opts;

	if (opts->present & DL_OPT_HANDLE) {
		mnl_attr_put_strz(nlh, DEVLINK_ATTR_BUS_NAME, opts->bus_name);
		mnl_attr_put_strz(nlh, DEVLINK_ATTR_DEV_NAME, opts->dev_name);
	} else if (opts->present & DL_OPT_HANDLEP) {
		mnl_attr_put_strz(nlh, DEVLINK_ATTR_BUS_NAME, opts->bus_name);
		mnl_attr_put_strz(nlh, DEVLINK_ATTR_DEV_NAME, opts->dev_name);
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_INDEX,
				 opts->port_index);
	}
	if (opts->present & DL_OPT_PORT_TYPE)
		mnl_attr_put_u16(nlh, DEVLINK_ATTR_PORT_TYPE,
				 opts->port_type);
	if (opts->present & DL_OPT_PORT_COUNT)
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_SPLIT_COUNT,
				 opts->port_count);
	if (opts->present & DL_OPT_SB)
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_SB_INDEX,
				 opts->sb_index);
	if (opts->present & DL_OPT_SB_POOL)
		mnl_attr_put_u16(nlh, DEVLINK_ATTR_SB_POOL_INDEX,
				 opts->sb_pool_index);
	if (opts->present & DL_OPT_SB_SIZE)
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_SB_POOL_SIZE,
				 opts->sb_pool_size);
	if (opts->present & DL_OPT_SB_TYPE)
		mnl_attr_put_u8(nlh, DEVLINK_ATTR_SB_POOL_TYPE,
				opts->sb_pool_type);
	if (opts->present & DL_OPT_SB_THTYPE)
		mnl_attr_put_u8(nlh, DEVLINK_ATTR_SB_POOL_THRESHOLD_TYPE,
				opts->sb_pool_thtype);
	if (opts->present & DL_OPT_SB_TH)
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_SB_THRESHOLD,
				 opts->sb_threshold);
	if (opts->present & DL_OPT_SB_TC)
		mnl_attr_put_u16(nlh, DEVLINK_ATTR_SB_TC_INDEX,
				 opts->sb_tc_index);
	if (opts->present & DL_OPT_ESWITCH_MODE)
		mnl_attr_put_u16(nlh, DEVLINK_ATTR_ESWITCH_MODE,
				 opts->eswitch_mode);
	if (opts->present & DL_OPT_ESWITCH_INLINE_MODE)
		mnl_attr_put_u8(nlh, DEVLINK_ATTR_ESWITCH_INLINE_MODE,
				opts->eswitch_inline_mode);
}

static int dl_argv_parse_put(struct nlmsghdr *nlh, struct dl *dl,
			     uint32_t o_required, uint32_t o_optional)
{
	int err;

	err = dl_argv_parse(dl, o_required, o_optional);
	if (err)
		return err;
	dl_opts_put(nlh, dl);
	return 0;
}

static bool dl_dump_filter(struct dl *dl, struct nlattr **tb)
{
	struct dl_opts *opts = &dl->opts;
	struct nlattr *attr_bus_name = tb[DEVLINK_ATTR_BUS_NAME];
	struct nlattr *attr_dev_name = tb[DEVLINK_ATTR_DEV_NAME];
	struct nlattr *attr_port_index = tb[DEVLINK_ATTR_PORT_INDEX];
	struct nlattr *attr_sb_index = tb[DEVLINK_ATTR_SB_INDEX];

	if (opts->present & DL_OPT_HANDLE &&
	    attr_bus_name && attr_dev_name) {
		const char *bus_name = mnl_attr_get_str(attr_bus_name);
		const char *dev_name = mnl_attr_get_str(attr_dev_name);

		if (strcmp(bus_name, opts->bus_name) != 0 ||
		    strcmp(dev_name, opts->dev_name) != 0)
			return false;
	}
	if (opts->present & DL_OPT_HANDLEP &&
	    attr_bus_name && attr_dev_name && attr_port_index) {
		const char *bus_name = mnl_attr_get_str(attr_bus_name);
		const char *dev_name = mnl_attr_get_str(attr_dev_name);
		uint32_t port_index = mnl_attr_get_u32(attr_port_index);

		if (strcmp(bus_name, opts->bus_name) != 0 ||
		    strcmp(dev_name, opts->dev_name) != 0 ||
		    port_index != opts->port_index)
			return false;
	}
	if (opts->present & DL_OPT_SB && attr_sb_index) {
		uint32_t sb_index = mnl_attr_get_u32(attr_sb_index);

		if (sb_index != opts->sb_index)
			return false;
	}
	return true;
}

static void cmd_dev_help(void)
{
	pr_err("Usage: devlink dev show [ DEV ]\n");
	pr_err("       devlink dev eswitch set DEV [ mode { legacy | switchdev } ]\n");
	pr_err("                               [ inline-mode { none | link | network | transport } ]\n");
	pr_err("       devlink dev eswitch show DEV\n");
}

static bool cmp_arr_last_handle(struct dl *dl, const char *bus_name,
				const char *dev_name)
{
	if (!dl->arr_last.present)
		return false;
	return strcmp(dl->arr_last.bus_name, bus_name) == 0 &&
	       strcmp(dl->arr_last.dev_name, dev_name) == 0;
}

static void arr_last_handle_set(struct dl *dl, const char *bus_name,
				const char *dev_name)
{
	dl->arr_last.present = true;
	free(dl->arr_last.dev_name);
	free(dl->arr_last.bus_name);
	dl->arr_last.bus_name = strdup(bus_name);
	dl->arr_last.dev_name = strdup(dev_name);
}

static bool should_arr_last_handle_start(struct dl *dl, const char *bus_name,
					 const char *dev_name)
{
	return !cmp_arr_last_handle(dl, bus_name, dev_name);
}

static bool should_arr_last_handle_end(struct dl *dl, const char *bus_name,
				       const char *dev_name)
{
	return dl->arr_last.present &&
	       !cmp_arr_last_handle(dl, bus_name, dev_name);
}

static void __pr_out_handle_start(struct dl *dl, struct nlattr **tb,
				  bool content, bool array)
{
	const char *bus_name = mnl_attr_get_str(tb[DEVLINK_ATTR_BUS_NAME]);
	const char *dev_name = mnl_attr_get_str(tb[DEVLINK_ATTR_DEV_NAME]);
	char buf[32];

	sprintf(buf, "%s/%s", bus_name, dev_name);

	if (dl->json_output) {
		if (array) {
			if (should_arr_last_handle_end(dl, bus_name, dev_name))
				jsonw_end_array(dl->jw);
			if (should_arr_last_handle_start(dl, bus_name,
							 dev_name)) {
				jsonw_name(dl->jw, buf);
				jsonw_start_array(dl->jw);
				jsonw_start_object(dl->jw);
				arr_last_handle_set(dl, bus_name, dev_name);
			} else {
				jsonw_start_object(dl->jw);
			}
		} else {
			jsonw_name(dl->jw, buf);
			jsonw_start_object(dl->jw);
		}
	} else {
		pr_out("%s%s", buf, content ? ":" : "");
	}
}

static void pr_out_handle_start_arr(struct dl *dl, struct nlattr **tb)
{
	__pr_out_handle_start(dl, tb, true, true);
}

static void pr_out_handle_end(struct dl *dl)
{
	if (dl->json_output)
		jsonw_end_object(dl->jw);
	else
		pr_out("\n");
}

static void pr_out_handle(struct dl *dl, struct nlattr **tb)
{
	__pr_out_handle_start(dl, tb, false, false);
	pr_out_handle_end(dl);
}

static bool cmp_arr_last_port_handle(struct dl *dl, const char *bus_name,
				     const char *dev_name, uint32_t port_index)
{
	return cmp_arr_last_handle(dl, bus_name, dev_name) &&
	       dl->arr_last.port_index == port_index;
}

static void arr_last_port_handle_set(struct dl *dl, const char *bus_name,
				     const char *dev_name, uint32_t port_index)
{
	arr_last_handle_set(dl, bus_name, dev_name);
	dl->arr_last.port_index = port_index;
}

static bool should_arr_last_port_handle_start(struct dl *dl,
					      const char *bus_name,
					      const char *dev_name,
					      uint32_t port_index)
{
	return !cmp_arr_last_port_handle(dl, bus_name, dev_name, port_index);
}

static bool should_arr_last_port_handle_end(struct dl *dl,
					    const char *bus_name,
					    const char *dev_name,
					    uint32_t port_index)
{
	return dl->arr_last.present &&
	       !cmp_arr_last_port_handle(dl, bus_name, dev_name, port_index);
}

static void __pr_out_port_handle_start(struct dl *dl, const char *bus_name,
				       const char *dev_name,
				       uint32_t port_index, bool try_nice,
				       bool array)
{
	static char buf[32];
	char *ifname = NULL;

	if (dl->no_nice_names || !try_nice ||
	    ifname_map_rev_lookup(dl, bus_name, dev_name,
				  port_index, &ifname) != 0)
		sprintf(buf, "%s/%s/%d", bus_name, dev_name, port_index);
	else
		sprintf(buf, "%s", ifname);

	if (dl->json_output) {
		if (array) {
			if (should_arr_last_port_handle_end(dl, bus_name,
							    dev_name,
							    port_index))
				jsonw_end_array(dl->jw);
			if (should_arr_last_port_handle_start(dl, bus_name,
							      dev_name,
							      port_index)) {
				jsonw_name(dl->jw, buf);
				jsonw_start_array(dl->jw);
				jsonw_start_object(dl->jw);
				arr_last_port_handle_set(dl, bus_name, dev_name,
							 port_index);
			} else {
				jsonw_start_object(dl->jw);
			}
		} else {
			jsonw_name(dl->jw, buf);
			jsonw_start_object(dl->jw);
		}
	} else {
		pr_out("%s:", buf);
	}
}

static void pr_out_port_handle_start(struct dl *dl, struct nlattr **tb, bool try_nice)
{
	const char *bus_name;
	const char *dev_name;
	uint32_t port_index;

	bus_name = mnl_attr_get_str(tb[DEVLINK_ATTR_BUS_NAME]);
	dev_name = mnl_attr_get_str(tb[DEVLINK_ATTR_DEV_NAME]);
	port_index = mnl_attr_get_u32(tb[DEVLINK_ATTR_PORT_INDEX]);
	__pr_out_port_handle_start(dl, bus_name, dev_name, port_index, try_nice, false);
}

static void pr_out_port_handle_start_arr(struct dl *dl, struct nlattr **tb, bool try_nice)
{
	const char *bus_name;
	const char *dev_name;
	uint32_t port_index;

	bus_name = mnl_attr_get_str(tb[DEVLINK_ATTR_BUS_NAME]);
	dev_name = mnl_attr_get_str(tb[DEVLINK_ATTR_DEV_NAME]);
	port_index = mnl_attr_get_u32(tb[DEVLINK_ATTR_PORT_INDEX]);
	__pr_out_port_handle_start(dl, bus_name, dev_name, port_index, try_nice, true);
}

static void pr_out_port_handle_end(struct dl *dl)
{
	if (dl->json_output)
		jsonw_end_object(dl->jw);
	else
		pr_out("\n");
}


static void pr_out_str(struct dl *dl, const char *name, const char *val)
{
	if (dl->json_output)
		jsonw_string_field(dl->jw, name, val);
	else
		pr_out(" %s %s", name, val);
}

static void pr_out_uint(struct dl *dl, const char *name, unsigned int val)
{
	if (dl->json_output)
		jsonw_uint_field(dl->jw, name, val);
	else
		pr_out(" %s %u", name, val);
}

static void pr_out_dev(struct dl *dl, struct nlattr **tb)
{
	pr_out_handle(dl, tb);
}

static void pr_out_section_start(struct dl *dl, const char *name)
{
	if (dl->json_output) {
		jsonw_start_object(dl->jw);
		jsonw_name(dl->jw, name);
		jsonw_start_object(dl->jw);
	}
}

static void pr_out_section_end(struct dl *dl)
{
	if (dl->json_output) {
		if (dl->arr_last.present)
			jsonw_end_array(dl->jw);
		jsonw_end_object(dl->jw);
		jsonw_end_object(dl->jw);
	}
}

static const char *eswitch_mode_name(uint32_t mode)
{
	switch (mode) {
	case DEVLINK_ESWITCH_MODE_LEGACY: return ESWITCH_MODE_LEGACY;
	case DEVLINK_ESWITCH_MODE_SWITCHDEV: return ESWITCH_MODE_SWITCHDEV;
	default: return "<unknown mode>";
	}
}

static const char *eswitch_inline_mode_name(uint32_t mode)
{
	switch (mode) {
	case DEVLINK_ESWITCH_INLINE_MODE_NONE:
		return ESWITCH_INLINE_MODE_NONE;
	case DEVLINK_ESWITCH_INLINE_MODE_LINK:
		return ESWITCH_INLINE_MODE_LINK;
	case DEVLINK_ESWITCH_INLINE_MODE_NETWORK:
		return ESWITCH_INLINE_MODE_NETWORK;
	case DEVLINK_ESWITCH_INLINE_MODE_TRANSPORT:
		return ESWITCH_INLINE_MODE_TRANSPORT;
	default:
		return "<unknown mode>";
	}
}

static void pr_out_eswitch(struct dl *dl, struct nlattr **tb)
{
	__pr_out_handle_start(dl, tb, true, false);

	if (tb[DEVLINK_ATTR_ESWITCH_MODE])
		pr_out_str(dl, "mode",
			   eswitch_mode_name(mnl_attr_get_u16(tb[DEVLINK_ATTR_ESWITCH_MODE])));

	if (tb[DEVLINK_ATTR_ESWITCH_INLINE_MODE])
		pr_out_str(dl, "inline-mode",
			   eswitch_inline_mode_name(mnl_attr_get_u8(
				   tb[DEVLINK_ATTR_ESWITCH_INLINE_MODE])));

	pr_out_handle_end(dl);
}

static int cmd_dev_eswitch_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME])
		return MNL_CB_ERROR;
	pr_out_eswitch(dl, tb);
	return MNL_CB_OK;
}

static int cmd_dev_eswitch_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_ESWITCH_MODE_GET,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE, 0);
	if (err)
		return err;

	pr_out_section_start(dl, "dev");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_dev_eswitch_show_cb, dl);
	pr_out_section_end(dl);
	return err;
}

static int cmd_dev_eswitch_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_ESWITCH_MODE_SET,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE,
				DL_OPT_ESWITCH_MODE |
				DL_OPT_ESWITCH_INLINE_MODE);

	if (err)
		return err;

	if (dl->opts.present == 1) {
		pr_err("Need to set at least one option\n");
		return -ENOENT;
	}

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_dev_eswitch(struct dl *dl)
{
	if (dl_argv_match(dl, "help") || dl_no_arg(dl)) {
		cmd_dev_help();
		return 0;
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_dev_eswitch_set(dl);
	} else if (dl_argv_match(dl, "show")) {
		dl_arg_inc(dl);
		return cmd_dev_eswitch_show(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static int cmd_dev_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME])
		return MNL_CB_ERROR;
	pr_out_dev(dl, tb);
	return MNL_CB_OK;
}

static int cmd_dev_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_GET, flags);

	if (dl_argc(dl) > 0) {
		err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE, 0);
		if (err)
			return err;
	}

	pr_out_section_start(dl, "dev");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_dev_show_cb, dl);
	pr_out_section_end(dl);
	return err;
}

static int cmd_dev(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_dev_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_dev_show(dl);
	} else if (dl_argv_match(dl, "eswitch")) {
		dl_arg_inc(dl);
		return cmd_dev_eswitch(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static void cmd_port_help(void)
{
	pr_err("Usage: devlink port show [ DEV/PORT_INDEX ]\n");
	pr_err("       devlink port set DEV/PORT_INDEX [ type { eth | ib | auto} ]\n");
	pr_err("       devlink port split DEV/PORT_INDEX count COUNT\n");
	pr_err("       devlink port unsplit DEV/PORT_INDEX\n");
}

static const char *port_type_name(uint32_t type)
{
	switch (type) {
	case DEVLINK_PORT_TYPE_NOTSET: return "notset";
	case DEVLINK_PORT_TYPE_AUTO: return "auto";
	case DEVLINK_PORT_TYPE_ETH: return "eth";
	case DEVLINK_PORT_TYPE_IB: return "ib";
	default: return "<unknown type>";
	}
}

static void pr_out_port(struct dl *dl, struct nlattr **tb)
{
	struct nlattr *pt_attr = tb[DEVLINK_ATTR_PORT_TYPE];
	struct nlattr *dpt_attr = tb[DEVLINK_ATTR_PORT_DESIRED_TYPE];

	pr_out_port_handle_start(dl, tb, false);
	if (pt_attr) {
		uint16_t port_type = mnl_attr_get_u16(pt_attr);

		pr_out_str(dl, "type", port_type_name(port_type));
		if (dpt_attr) {
			uint16_t des_port_type = mnl_attr_get_u16(dpt_attr);

			if (port_type != des_port_type)
				pr_out_str(dl, "des_type",
					   port_type_name(des_port_type));
		}
	}
	if (tb[DEVLINK_ATTR_PORT_NETDEV_NAME])
		pr_out_str(dl, "netdev",
			   mnl_attr_get_str(tb[DEVLINK_ATTR_PORT_NETDEV_NAME]));
	if (tb[DEVLINK_ATTR_PORT_IBDEV_NAME])
		pr_out_str(dl, "ibdev",
			   mnl_attr_get_str(tb[DEVLINK_ATTR_PORT_IBDEV_NAME]));
	if (tb[DEVLINK_ATTR_PORT_SPLIT_GROUP])
		pr_out_uint(dl, "split_group",
			    mnl_attr_get_u32(tb[DEVLINK_ATTR_PORT_SPLIT_GROUP]));
	pr_out_port_handle_end(dl);
}

static int cmd_port_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_PORT_INDEX])
		return MNL_CB_ERROR;
	pr_out_port(dl, tb);
	return MNL_CB_OK;
}

static int cmd_port_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_GET, flags);

	if (dl_argc(dl) > 0) {
		err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP, 0);
		if (err)
			return err;
	}

	pr_out_section_start(dl, "port");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_port_show_cb, dl);
	pr_out_section_end(dl);
	return err;
}

static int cmd_port_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_SET,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP | DL_OPT_PORT_TYPE, 0);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_port_split(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_SPLIT,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP | DL_OPT_PORT_COUNT, 0);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_port_unsplit(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_UNSPLIT,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP, 0);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_port(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_port_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list") ||  dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_port_show(dl);
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_port_set(dl);
	} else if (dl_argv_match(dl, "split")) {
		dl_arg_inc(dl);
		return cmd_port_split(dl);
	} else if (dl_argv_match(dl, "unsplit")) {
		dl_arg_inc(dl);
		return cmd_port_unsplit(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static void cmd_sb_help(void)
{
	pr_err("Usage: devlink sb show [ DEV [ sb SB_INDEX ] ]\n");
	pr_err("       devlink sb pool show [ DEV [ sb SB_INDEX ] pool POOL_INDEX ]\n");
	pr_err("       devlink sb pool set DEV [ sb SB_INDEX ] pool POOL_INDEX\n");
	pr_err("                           size POOL_SIZE thtype { static | dynamic }\n");
	pr_err("       devlink sb port pool show [ DEV/PORT_INDEX [ sb SB_INDEX ]\n");
	pr_err("                                   pool POOL_INDEX ]\n");
	pr_err("       devlink sb port pool set DEV/PORT_INDEX [ sb SB_INDEX ]\n");
	pr_err("                                pool POOL_INDEX th THRESHOLD\n");
	pr_err("       devlink sb tc bind show [ DEV/PORT_INDEX [ sb SB_INDEX ] tc TC_INDEX\n");
	pr_err("                                 type { ingress | egress } ]\n");
	pr_err("       devlink sb tc bind set DEV/PORT_INDEX [ sb SB_INDEX ] tc TC_INDEX\n");
	pr_err("                              type { ingress | egress } pool POOL_INDEX\n");
	pr_err("                              th THRESHOLD\n");
	pr_err("       devlink sb occupancy show { DEV | DEV/PORT_INDEX } [ sb SB_INDEX ]\n");
	pr_err("       devlink sb occupancy snapshot DEV [ sb SB_INDEX ]\n");
	pr_err("       devlink sb occupancy clearmax DEV [ sb SB_INDEX ]\n");
}

static void pr_out_sb(struct dl *dl, struct nlattr **tb)
{
	pr_out_handle_start_arr(dl, tb);
	pr_out_uint(dl, "sb",
		    mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_INDEX]));
	pr_out_uint(dl, "size",
		    mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_SIZE]));
	pr_out_uint(dl, "ing_pools",
		    mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_INGRESS_POOL_COUNT]));
	pr_out_uint(dl, "eg_pools",
		    mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_EGRESS_POOL_COUNT]));
	pr_out_uint(dl, "ing_tcs",
		    mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_INGRESS_TC_COUNT]));
	pr_out_uint(dl, "eg_tcs",
		    mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_EGRESS_TC_COUNT]));
	pr_out_handle_end(dl);
}

static int cmd_sb_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_SB_INDEX] || !tb[DEVLINK_ATTR_SB_SIZE] ||
	    !tb[DEVLINK_ATTR_SB_INGRESS_POOL_COUNT] ||
	    !tb[DEVLINK_ATTR_SB_EGRESS_POOL_COUNT] ||
	    !tb[DEVLINK_ATTR_SB_INGRESS_TC_COUNT] ||
	    !tb[DEVLINK_ATTR_SB_EGRESS_TC_COUNT])
		return MNL_CB_ERROR;
	pr_out_sb(dl, tb);
	return MNL_CB_OK;
}

static int cmd_sb_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_GET, flags);

	if (dl_argc(dl) > 0) {
		err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE, DL_OPT_SB);
		if (err)
			return err;
	}

	pr_out_section_start(dl, "sb");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_sb_show_cb, dl);
	pr_out_section_end(dl);
	return err;
}

static const char *pool_type_name(uint8_t type)
{
	switch (type) {
	case DEVLINK_SB_POOL_TYPE_INGRESS: return "ingress";
	case DEVLINK_SB_POOL_TYPE_EGRESS: return "egress";
	default: return "<unknown type>";
	}
}

static const char *threshold_type_name(uint8_t type)
{
	switch (type) {
	case DEVLINK_SB_THRESHOLD_TYPE_STATIC: return "static";
	case DEVLINK_SB_THRESHOLD_TYPE_DYNAMIC: return "dynamic";
	default: return "<unknown type>";
	}
}

static void pr_out_sb_pool(struct dl *dl, struct nlattr **tb)
{
	pr_out_handle_start_arr(dl, tb);
	pr_out_uint(dl, "sb",
		    mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_INDEX]));
	pr_out_uint(dl, "pool",
		    mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_POOL_INDEX]));
	pr_out_str(dl, "type",
		   pool_type_name(mnl_attr_get_u8(tb[DEVLINK_ATTR_SB_POOL_TYPE])));
	pr_out_uint(dl, "size",
		    mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_POOL_SIZE]));
	pr_out_str(dl, "thtype",
		   threshold_type_name(mnl_attr_get_u8(tb[DEVLINK_ATTR_SB_POOL_THRESHOLD_TYPE])));
	pr_out_handle_end(dl);
}

static int cmd_sb_pool_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_SB_INDEX] || !tb[DEVLINK_ATTR_SB_POOL_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_POOL_TYPE] || !tb[DEVLINK_ATTR_SB_POOL_SIZE] ||
	    !tb[DEVLINK_ATTR_SB_POOL_THRESHOLD_TYPE])
		return MNL_CB_ERROR;
	pr_out_sb_pool(dl, tb);
	return MNL_CB_OK;
}

static int cmd_sb_pool_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_POOL_GET, flags);

	if (dl_argc(dl) > 0) {
		err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE | DL_OPT_SB_POOL,
					DL_OPT_SB);
		if (err)
			return err;
	}

	pr_out_section_start(dl, "pool");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_sb_pool_show_cb, dl);
	pr_out_section_end(dl);
	return err;
}

static int cmd_sb_pool_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_POOL_SET,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE | DL_OPT_SB_POOL |
				DL_OPT_SB_SIZE | DL_OPT_SB_THTYPE, DL_OPT_SB);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_sb_pool(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_sb_pool_show(dl);
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_sb_pool_set(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static void pr_out_sb_port_pool(struct dl *dl, struct nlattr **tb)
{
	pr_out_port_handle_start_arr(dl, tb, true);
	pr_out_uint(dl, "sb",
		    mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_INDEX]));
	pr_out_uint(dl, "pool",
		    mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_POOL_INDEX]));
	pr_out_uint(dl, "threshold",
		    mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_THRESHOLD]));
	pr_out_port_handle_end(dl);
}

static int cmd_sb_port_pool_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_PORT_INDEX] || !tb[DEVLINK_ATTR_SB_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_POOL_INDEX] || !tb[DEVLINK_ATTR_SB_THRESHOLD])
		return MNL_CB_ERROR;
	pr_out_sb_port_pool(dl, tb);
	return MNL_CB_OK;
}

static int cmd_sb_port_pool_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_PORT_POOL_GET, flags);

	if (dl_argc(dl) > 0) {
		err = dl_argv_parse_put(nlh, dl,
					DL_OPT_HANDLEP | DL_OPT_SB_POOL,
					DL_OPT_SB);
		if (err)
			return err;
	}

	pr_out_section_start(dl, "port_pool");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_sb_port_pool_show_cb, dl);
	pr_out_section_end(dl);
	return 0;
}

static int cmd_sb_port_pool_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_PORT_POOL_SET,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP | DL_OPT_SB_POOL |
				DL_OPT_SB_TH, DL_OPT_SB);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_sb_port_pool(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_sb_port_pool_show(dl);
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_sb_port_pool_set(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static int cmd_sb_port(struct dl *dl)
{
	if (dl_argv_match(dl, "help") || dl_no_arg(dl)) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "pool")) {
		dl_arg_inc(dl);
		return cmd_sb_port_pool(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static void pr_out_sb_tc_bind(struct dl *dl, struct nlattr **tb)
{
	pr_out_port_handle_start_arr(dl, tb, true);
	pr_out_uint(dl, "sb",
	       mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_INDEX]));
	pr_out_uint(dl, "tc",
	       mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_TC_INDEX]));
	pr_out_str(dl, "type",
	       pool_type_name(mnl_attr_get_u8(tb[DEVLINK_ATTR_SB_POOL_TYPE])));
	pr_out_uint(dl, "pool",
	       mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_POOL_INDEX]));
	pr_out_uint(dl, "threshold",
	       mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_THRESHOLD]));
	pr_out_port_handle_end(dl);
}

static int cmd_sb_tc_bind_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_PORT_INDEX] || !tb[DEVLINK_ATTR_SB_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_TC_INDEX] || !tb[DEVLINK_ATTR_SB_POOL_TYPE] ||
	    !tb[DEVLINK_ATTR_SB_POOL_INDEX] || !tb[DEVLINK_ATTR_SB_THRESHOLD])
		return MNL_CB_ERROR;
	pr_out_sb_tc_bind(dl, tb);
	return MNL_CB_OK;
}

static int cmd_sb_tc_bind_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_TC_POOL_BIND_GET, flags);

	if (dl_argc(dl) > 0) {
		err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP | DL_OPT_SB_TC |
					DL_OPT_SB_TYPE, DL_OPT_SB);
		if (err)
			return err;
	}

	pr_out_section_start(dl, "tc_bind");
	err = _mnlg_socket_sndrcv(dl->nlg, nlh, cmd_sb_tc_bind_show_cb, dl);
	pr_out_section_end(dl);
	return err;
}

static int cmd_sb_tc_bind_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_TC_POOL_BIND_SET,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLEP | DL_OPT_SB_TC |
				DL_OPT_SB_TYPE | DL_OPT_SB_POOL | DL_OPT_SB_TH,
				DL_OPT_SB);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_sb_tc_bind(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_sb_tc_bind_show(dl);
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_sb_tc_bind_set(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static int cmd_sb_tc(struct dl *dl)
{
	if (dl_argv_match(dl, "help") || dl_no_arg(dl)) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "bind")) {
		dl_arg_inc(dl);
		return cmd_sb_tc_bind(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

struct occ_item {
	struct list_head list;
	uint32_t index;
	uint32_t cur;
	uint32_t max;
	uint32_t bound_pool_index;
};

struct occ_port {
	struct list_head list;
	char *bus_name;
	char *dev_name;
	uint32_t port_index;
	uint32_t sb_index;
	struct list_head pool_list;
	struct list_head ing_tc_list;
	struct list_head eg_tc_list;
};

struct occ_show {
	struct dl *dl;
	int err;
	struct list_head port_list;
};

static struct occ_item *occ_item_alloc(void)
{
	return calloc(1, sizeof(struct occ_item));
}

static void occ_item_free(struct occ_item *occ_item)
{
	free(occ_item);
}

static struct occ_port *occ_port_alloc(uint32_t port_index)
{
	struct occ_port *occ_port;

	occ_port = calloc(1, sizeof(*occ_port));
	if (!occ_port)
		return NULL;
	occ_port->port_index = port_index;
	INIT_LIST_HEAD(&occ_port->pool_list);
	INIT_LIST_HEAD(&occ_port->ing_tc_list);
	INIT_LIST_HEAD(&occ_port->eg_tc_list);
	return occ_port;
}

static void occ_port_free(struct occ_port *occ_port)
{
	struct occ_item *occ_item, *tmp;

	list_for_each_entry_safe(occ_item, tmp, &occ_port->pool_list, list)
		occ_item_free(occ_item);
	list_for_each_entry_safe(occ_item, tmp, &occ_port->ing_tc_list, list)
		occ_item_free(occ_item);
	list_for_each_entry_safe(occ_item, tmp, &occ_port->eg_tc_list, list)
		occ_item_free(occ_item);
}

static struct occ_show *occ_show_alloc(struct dl *dl)
{
	struct occ_show *occ_show;

	occ_show = calloc(1, sizeof(*occ_show));
	if (!occ_show)
		return NULL;
	occ_show->dl = dl;
	INIT_LIST_HEAD(&occ_show->port_list);
	return occ_show;
}

static void occ_show_free(struct occ_show *occ_show)
{
	struct occ_port *occ_port, *tmp;

	list_for_each_entry_safe(occ_port, tmp, &occ_show->port_list, list)
		occ_port_free(occ_port);
}

static struct occ_port *occ_port_get(struct occ_show *occ_show,
				     struct nlattr **tb)
{
	struct occ_port *occ_port;
	uint32_t port_index;

	port_index = mnl_attr_get_u32(tb[DEVLINK_ATTR_PORT_INDEX]);

	list_for_each_entry_reverse(occ_port, &occ_show->port_list, list) {
		if (occ_port->port_index == port_index)
			return occ_port;
	}
	occ_port = occ_port_alloc(port_index);
	if (!occ_port)
		return NULL;
	list_add_tail(&occ_port->list, &occ_show->port_list);
	return occ_port;
}

static void pr_out_occ_show_item_list(const char *label, struct list_head *list,
				      bool bound_pool)
{
	struct occ_item *occ_item;
	int i = 1;

	pr_out_sp(7, "  %s:", label);
	list_for_each_entry(occ_item, list, list) {
		if ((i - 1) % 4 == 0 && i != 1)
			pr_out_sp(7, " ");
		if (bound_pool)
			pr_out_sp(7, "%2u(%u):", occ_item->index,
				  occ_item->bound_pool_index);
		else
			pr_out_sp(7, "%2u:", occ_item->index);
		pr_out_sp(15, "%7u/%u", occ_item->cur, occ_item->max);
		if (i++ % 4 == 0)
			pr_out("\n");
	}
	if ((i - 1) % 4 != 0)
		pr_out("\n");
}

static void pr_out_json_occ_show_item_list(struct dl *dl, const char *label,
					   struct list_head *list,
					   bool bound_pool)
{
	struct occ_item *occ_item;
	char buf[32];

	jsonw_name(dl->jw, label);
	jsonw_start_object(dl->jw);
	list_for_each_entry(occ_item, list, list) {
		sprintf(buf, "%u", occ_item->index);
		jsonw_name(dl->jw, buf);
		jsonw_start_object(dl->jw);
		if (bound_pool)
			jsonw_uint_field(dl->jw, "bound_pool",
					 occ_item->bound_pool_index);
		jsonw_uint_field(dl->jw, "current", occ_item->cur);
		jsonw_uint_field(dl->jw, "max", occ_item->max);
		jsonw_end_object(dl->jw);
	}
	jsonw_end_object(dl->jw);
}

static void pr_out_occ_show_port(struct dl *dl, struct occ_port *occ_port)
{
	if (dl->json_output) {
		pr_out_json_occ_show_item_list(dl, "pool",
					       &occ_port->pool_list, false);
		pr_out_json_occ_show_item_list(dl, "itc",
					       &occ_port->ing_tc_list, true);
		pr_out_json_occ_show_item_list(dl, "etc",
					       &occ_port->eg_tc_list, true);
	} else {
		pr_out("\n");
		pr_out_occ_show_item_list("pool", &occ_port->pool_list, false);
		pr_out_occ_show_item_list("itc", &occ_port->ing_tc_list, true);
		pr_out_occ_show_item_list("etc", &occ_port->eg_tc_list, true);
	}
}

static void pr_out_occ_show(struct occ_show *occ_show)
{
	struct dl *dl = occ_show->dl;
	struct dl_opts *opts = &dl->opts;
	struct occ_port *occ_port;

	list_for_each_entry(occ_port, &occ_show->port_list, list) {
		__pr_out_port_handle_start(dl, opts->bus_name, opts->dev_name,
					   occ_port->port_index, true, false);
		pr_out_occ_show_port(dl, occ_port);
		pr_out_port_handle_end(dl);
	}
}

static void cmd_sb_occ_port_pool_process(struct occ_show *occ_show,
					 struct nlattr **tb)
{
	struct occ_port *occ_port;
	struct occ_item *occ_item;

	if (occ_show->err || !dl_dump_filter(occ_show->dl, tb))
		return;

	occ_port = occ_port_get(occ_show, tb);
	if (!occ_port) {
		occ_show->err = -ENOMEM;
		return;
	}

	occ_item = occ_item_alloc();
	if (!occ_item) {
		occ_show->err = -ENOMEM;
		return;
	}
	occ_item->index = mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_POOL_INDEX]);
	occ_item->cur = mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_OCC_CUR]);
	occ_item->max = mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_OCC_MAX]);
	list_add_tail(&occ_item->list, &occ_port->pool_list);
}

static int cmd_sb_occ_port_pool_process_cb(const struct nlmsghdr *nlh, void *data)
{
	struct occ_show *occ_show = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_PORT_INDEX] || !tb[DEVLINK_ATTR_SB_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_POOL_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_OCC_CUR] || !tb[DEVLINK_ATTR_SB_OCC_MAX])
		return MNL_CB_ERROR;
	cmd_sb_occ_port_pool_process(occ_show, tb);
	return MNL_CB_OK;
}

static void cmd_sb_occ_tc_pool_process(struct occ_show *occ_show,
				       struct nlattr **tb)
{
	struct occ_port *occ_port;
	struct occ_item *occ_item;
	uint8_t pool_type;

	if (occ_show->err || !dl_dump_filter(occ_show->dl, tb))
		return;

	occ_port = occ_port_get(occ_show, tb);
	if (!occ_port) {
		occ_show->err = -ENOMEM;
		return;
	}

	occ_item = occ_item_alloc();
	if (!occ_item) {
		occ_show->err = -ENOMEM;
		return;
	}
	occ_item->index = mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_TC_INDEX]);
	occ_item->cur = mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_OCC_CUR]);
	occ_item->max = mnl_attr_get_u32(tb[DEVLINK_ATTR_SB_OCC_MAX]);
	occ_item->bound_pool_index =
			mnl_attr_get_u16(tb[DEVLINK_ATTR_SB_POOL_INDEX]);
	pool_type = mnl_attr_get_u8(tb[DEVLINK_ATTR_SB_POOL_TYPE]);
	if (pool_type == DEVLINK_SB_POOL_TYPE_INGRESS)
		list_add_tail(&occ_item->list, &occ_port->ing_tc_list);
	else if (pool_type == DEVLINK_SB_POOL_TYPE_EGRESS)
		list_add_tail(&occ_item->list, &occ_port->eg_tc_list);
	else
		occ_item_free(occ_item);
}

static int cmd_sb_occ_tc_pool_process_cb(const struct nlmsghdr *nlh, void *data)
{
	struct occ_show *occ_show = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
	    !tb[DEVLINK_ATTR_PORT_INDEX] || !tb[DEVLINK_ATTR_SB_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_TC_INDEX] || !tb[DEVLINK_ATTR_SB_POOL_TYPE] ||
	    !tb[DEVLINK_ATTR_SB_POOL_INDEX] ||
	    !tb[DEVLINK_ATTR_SB_OCC_CUR] || !tb[DEVLINK_ATTR_SB_OCC_MAX])
		return MNL_CB_ERROR;
	cmd_sb_occ_tc_pool_process(occ_show, tb);
	return MNL_CB_OK;
}

static int cmd_sb_occ_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	struct occ_show *occ_show;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP;
	int err;

	err = dl_argv_parse(dl, DL_OPT_HANDLE | DL_OPT_HANDLEP, DL_OPT_SB);
	if (err)
		return err;

	occ_show = occ_show_alloc(dl);
	if (!occ_show)
		return -ENOMEM;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_PORT_POOL_GET, flags);

	err = _mnlg_socket_sndrcv(dl->nlg, nlh,
				  cmd_sb_occ_port_pool_process_cb, occ_show);
	if (err)
		goto out;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_TC_POOL_BIND_GET, flags);

	err = _mnlg_socket_sndrcv(dl->nlg, nlh,
				  cmd_sb_occ_tc_pool_process_cb, occ_show);
	if (err)
		goto out;

	pr_out_section_start(dl, "occupancy");
	pr_out_occ_show(occ_show);
	pr_out_section_end(dl);

out:
	occ_show_free(occ_show);
	return err;
}

static int cmd_sb_occ_snapshot(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_OCC_SNAPSHOT,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE, DL_OPT_SB);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_sb_occ_clearmax(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SB_OCC_MAX_CLEAR,
			       NLM_F_REQUEST | NLM_F_ACK);

	err = dl_argv_parse_put(nlh, dl, DL_OPT_HANDLE, DL_OPT_SB);
	if (err)
		return err;

	return _mnlg_socket_sndrcv(dl->nlg, nlh, NULL, NULL);
}

static int cmd_sb_occ(struct dl *dl)
{
	if (dl_argv_match(dl, "help") || dl_no_arg(dl)) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list")) {
		dl_arg_inc(dl);
		return cmd_sb_occ_show(dl);
	} else if (dl_argv_match(dl, "snapshot")) {
		dl_arg_inc(dl);
		return cmd_sb_occ_snapshot(dl);
	} else if (dl_argv_match(dl, "clearmax")) {
		dl_arg_inc(dl);
		return cmd_sb_occ_clearmax(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static int cmd_sb(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_sb_help();
		return 0;
	} else if (dl_argv_match(dl, "show") ||
		   dl_argv_match(dl, "list") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_sb_show(dl);
	} else if (dl_argv_match(dl, "pool")) {
		dl_arg_inc(dl);
		return cmd_sb_pool(dl);
	} else if (dl_argv_match(dl, "port")) {
		dl_arg_inc(dl);
		return cmd_sb_port(dl);
	} else if (dl_argv_match(dl, "tc")) {
		dl_arg_inc(dl);
		return cmd_sb_tc(dl);
	} else if (dl_argv_match(dl, "occupancy")) {
		dl_arg_inc(dl);
		return cmd_sb_occ(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static const char *cmd_name(uint8_t cmd)
{
	switch (cmd) {
	case DEVLINK_CMD_UNSPEC: return "unspec";
	case DEVLINK_CMD_GET: return "get";
	case DEVLINK_CMD_SET: return "set";
	case DEVLINK_CMD_NEW: return "new";
	case DEVLINK_CMD_DEL: return "del";
	case DEVLINK_CMD_PORT_GET: return "get";
	case DEVLINK_CMD_PORT_SET: return "set";
	case DEVLINK_CMD_PORT_NEW: return "net";
	case DEVLINK_CMD_PORT_DEL: return "del";
	default: return "<unknown cmd>";
	}
}

static const char *cmd_obj(uint8_t cmd)
{
	switch (cmd) {
	case DEVLINK_CMD_UNSPEC: return "unspec";
	case DEVLINK_CMD_GET:
	case DEVLINK_CMD_SET:
	case DEVLINK_CMD_NEW:
	case DEVLINK_CMD_DEL:
		return "dev";
	case DEVLINK_CMD_PORT_GET:
	case DEVLINK_CMD_PORT_SET:
	case DEVLINK_CMD_PORT_NEW:
	case DEVLINK_CMD_PORT_DEL:
		return "port";
	default: return "<unknown obj>";
	}
}

static void pr_out_mon_header(uint8_t cmd)
{
	pr_out("[%s,%s] ", cmd_obj(cmd), cmd_name(cmd));
}

static bool cmd_filter_check(struct dl *dl, uint8_t cmd)
{
	const char *obj = cmd_obj(cmd);
	unsigned int index = 0;
	const char *cur_obj;

	if (dl_no_arg(dl))
		return true;
	while ((cur_obj = dl_argv_index(dl, index++))) {
		if (strcmp(cur_obj, obj) == 0 || strcmp(cur_obj, "all") == 0)
			return true;
	}
	return false;
}

static int cmd_mon_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
	uint8_t cmd = genl->cmd;

	if (!cmd_filter_check(dl, cmd))
		return MNL_CB_OK;

	switch (cmd) {
	case DEVLINK_CMD_GET: /* fall through */
	case DEVLINK_CMD_SET: /* fall through */
	case DEVLINK_CMD_NEW: /* fall through */
	case DEVLINK_CMD_DEL:
		mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
		if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME])
			return MNL_CB_ERROR;
		pr_out_mon_header(genl->cmd);
		pr_out_dev(dl, tb);
		break;
	case DEVLINK_CMD_PORT_GET: /* fall through */
	case DEVLINK_CMD_PORT_SET: /* fall through */
	case DEVLINK_CMD_PORT_NEW: /* fall through */
	case DEVLINK_CMD_PORT_DEL:
		mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
		if (!tb[DEVLINK_ATTR_BUS_NAME] || !tb[DEVLINK_ATTR_DEV_NAME] ||
		    !tb[DEVLINK_ATTR_PORT_INDEX])
			return MNL_CB_ERROR;
		pr_out_mon_header(genl->cmd);
		pr_out_port(dl, tb);
		break;
	}
	return MNL_CB_OK;
}

static int cmd_mon_show(struct dl *dl)
{
	int err;
	unsigned int index = 0;
	const char *cur_obj;

	while ((cur_obj = dl_argv_index(dl, index++))) {
		if (strcmp(cur_obj, "all") != 0 &&
		    strcmp(cur_obj, "dev") != 0 &&
		    strcmp(cur_obj, "port") != 0) {
			pr_err("Unknown object \"%s\"\n", cur_obj);
			return -EINVAL;
		}
	}
	err = _mnlg_socket_group_add(dl->nlg, DEVLINK_GENL_MCGRP_CONFIG_NAME);
	if (err)
		return err;
	err = _mnlg_socket_recv_run(dl->nlg, cmd_mon_show_cb, dl);
	if (err)
		return err;
	return 0;
}

static void cmd_mon_help(void)
{
	pr_err("Usage: devlink monitor [ all | OBJECT-LIST ]\n"
	       "where  OBJECT-LIST := { dev | port }\n");
}

static int cmd_mon(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_mon_help();
		return 0;
	} else if (dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_mon_show(dl);
	}
	pr_err("Command \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static void help(void)
{
	pr_err("Usage: devlink [ OPTIONS ] OBJECT { COMMAND | help }\n"
	       "where  OBJECT := { dev | port | sb | monitor }\n"
	       "       OPTIONS := { -V[ersion] | -n[no-nice-names] }\n");
}

static int dl_cmd(struct dl *dl)
{
	if (dl_argv_match(dl, "help") || dl_no_arg(dl)) {
		help();
		return 0;
	} else if (dl_argv_match(dl, "dev")) {
		dl_arg_inc(dl);
		return cmd_dev(dl);
	} else if (dl_argv_match(dl, "port")) {
		dl_arg_inc(dl);
		return cmd_port(dl);
	} else if (dl_argv_match(dl, "sb")) {
		dl_arg_inc(dl);
		return cmd_sb(dl);
	} else if (dl_argv_match(dl, "monitor")) {
		dl_arg_inc(dl);
		return cmd_mon(dl);
	}
	pr_err("Object \"%s\" not found\n", dl_argv(dl));
	return -ENOENT;
}

static int dl_init(struct dl *dl, int argc, char **argv)
{
	int err;

	dl->argc = argc;
	dl->argv = argv;

	dl->nlg = mnlg_socket_open(DEVLINK_GENL_NAME, DEVLINK_GENL_VERSION);
	if (!dl->nlg) {
		pr_err("Failed to connect to devlink Netlink\n");
		return -errno;
	}

	err = ifname_map_init(dl);
	if (err) {
		pr_err("Failed to create index map\n");
		goto err_ifname_map_create;
	}
	if (dl->json_output) {
		dl->jw = jsonw_new(stdout);
		if (!dl->jw) {
			pr_err("Failed to create JSON writer\n");
			goto err_json_new;
		}
		jsonw_pretty(dl->jw, dl->pretty_output);
	}
	return 0;

err_json_new:
	ifname_map_fini(dl);
err_ifname_map_create:
	mnlg_socket_close(dl->nlg);
	return err;
}

static void dl_fini(struct dl *dl)
{
	if (dl->json_output)
		jsonw_destroy(&dl->jw);
	ifname_map_fini(dl);
	mnlg_socket_close(dl->nlg);
}

static struct dl *dl_alloc(void)
{
	struct dl *dl;

	dl = calloc(1, sizeof(*dl));
	if (!dl)
		return NULL;
	return dl;
}

static void dl_free(struct dl *dl)
{
	free(dl);
}

int main(int argc, char **argv)
{
	static const struct option long_options[] = {
		{ "Version",		no_argument,		NULL, 'V' },
		{ "no-nice-names",	no_argument,		NULL, 'n' },
		{ "json",		no_argument,		NULL, 'j' },
		{ "pretty",		no_argument,		NULL, 'p' },
		{ NULL, 0, NULL, 0 }
	};
	struct dl *dl;
	int opt;
	int err;
	int ret;

	dl = dl_alloc();
	if (!dl) {
		pr_err("Failed to allocate memory for devlink\n");
		return EXIT_FAILURE;
	}

	while ((opt = getopt_long(argc, argv, "Vnjp",
				  long_options, NULL)) >= 0) {

		switch (opt) {
		case 'V':
			printf("devlink utility, iproute2-ss%s\n", SNAPSHOT);
			ret = EXIT_SUCCESS;
			goto dl_free;
		case 'n':
			dl->no_nice_names = true;
			break;
		case 'j':
			dl->json_output = true;
			break;
		case 'p':
			dl->pretty_output = true;
			break;
		default:
			pr_err("Unknown option.\n");
			help();
			ret = EXIT_FAILURE;
			goto dl_free;
		}
	}

	argc -= optind;
	argv += optind;

	err = dl_init(dl, argc, argv);
	if (err) {
		ret = EXIT_FAILURE;
		goto dl_free;
	}

	err = dl_cmd(dl);
	if (err) {
		ret = EXIT_FAILURE;
		goto dl_fini;
	}

	ret = EXIT_SUCCESS;

dl_fini:
	dl_fini(dl);
dl_free:
	dl_free(dl);

	return ret;
}
