/*
 * state.c - state handling tool
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/fs.h>
#include <mtd/mtd-abi.h>

#include <barebox-state/state.h>
#include <barebox-state.h>
#include <dt/dt.h>
#include <state.h>

#define BAREBOX_STATE_LOCKFILE "/run/barebox-state.lock"

struct state_variable;

static int __state_uint8_set(struct state_variable *var, const char *val);
static int __state_uint32_set(struct state_variable *var, const char *val);
static char *__state_uint32_get(struct state_variable *var);
static int __state_enum32_set(struct state_variable *sv, const char *val);
static char *__state_enum32_get(struct state_variable *var);
static void __state_enum32_info(struct state_variable *var);
static int __state_mac_set(struct state_variable *var, const char *val);
static char *__state_mac_get(struct state_variable *var);
static int __state_string_set(struct state_variable *var, const char *val);
static char *__state_string_get(struct state_variable *var);

struct variable_str_type {
	enum state_variable_type type;
	char *type_name;

	char *(*get)(struct state_variable *var);
	int (*set)(struct state_variable *var, const char *val);
	void (*info)(struct state_variable *var);
};

static struct variable_str_type types[] =  {
	{
		.type = STATE_VARIABLE_TYPE_UINT8,
		.type_name = "uint8",
		.set = __state_uint8_set,
		.get = __state_uint32_get,
	}, {
		.type = STATE_VARIABLE_TYPE_UINT32,
		.type_name = "uint32",
		.set = __state_uint32_set,
		.get = __state_uint32_get,
	}, {
		.type = STATE_VARIABLE_TYPE_ENUM32,
		.type_name = "enum32",
		.set = __state_enum32_set,
		.get = __state_enum32_get,
		.info = __state_enum32_info,
	}, {
		.type = STATE_VARIABLE_TYPE_MAC,
		.type_name = "mac",
		.set = __state_mac_set,
		.get = __state_mac_get,
	}, {
		.type = STATE_VARIABLE_TYPE_STRING,
		.type_name = "string",
		.set = __state_string_set,
		.get = __state_string_get,
	},
};

static struct variable_str_type *state_find_type(enum state_variable_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		if (type == types[i].type) {
			return &types[i];
		}
	}

	return NULL;
}

static int __state_uint32_set(struct state_variable *var, const char *val)
{
	struct state_uint32 *su32 = to_state_uint32(var);

	su32->value = strtoul(val, NULL, 0);

	return 0;
}

static int __state_uint8_set(struct state_variable *var, const char *val)
{
	struct state_uint32 *su32 = to_state_uint32(var);
	unsigned long num;

	num = strtoul(val, NULL, 0);
	if (num > UINT8_MAX)
		return -ERANGE;

	su32->value = num;

	return 0;
}

static char *__state_uint32_get(struct state_variable *var)
{
	struct state_uint32 *su32 = to_state_uint32(var);
	char *str;
	int ret;

	ret = asprintf(&str, "%u", su32->value);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	return str;
}


static int __state_enum32_set(struct state_variable *sv, const char *val)
{
	struct state_enum32 *enum32 = to_state_enum32(sv);
	int i;

	for (i = 0; i < enum32->num_names; i++) {
		if (!strcmp(enum32->names[i], val)) {
			enum32->value = i;
			return 0;
		}
	}

	return -EINVAL;
}

static char *__state_enum32_get(struct state_variable *var)
{
	struct state_enum32 *enum32 = to_state_enum32(var);
	char *str;
	int ret;

	ret = asprintf(&str, "%s", enum32->names[enum32->value]);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	return str;
}

static void __state_enum32_info(struct state_variable *var)
{
	struct state_enum32 *enum32 = to_state_enum32(var);
	int i;

	printf(", values=[");

	for (i = 0; i < enum32->num_names; i++)
		printf("%s%s", enum32->names[i],
				i == enum32->num_names - 1 ? "" : ",");
	printf("]");
}

static int string_to_ethaddr(const char *str, uint8_t enetaddr[6])
{
	int reg;
	char *e;

	if (!str || strlen(str) != 17) {
		memset(enetaddr, 0, 6);
		return -EINVAL;
	}

	if (str[2] != ':' || str[5] != ':' || str[8] != ':' ||
			str[11] != ':' || str[14] != ':')
		return -EINVAL;

	for (reg = 0; reg < 6; ++reg) {
		enetaddr[reg] = strtoul(str, &e, 16);
		str = e + 1;
	}

	return 0;
}

static int __state_mac_set(struct state_variable *var, const char *val)
{
	struct state_mac *mac = to_state_mac(var);
	uint8_t mac_save[6];
	int ret;

	ret = string_to_ethaddr(val, mac_save);
	if (ret)
		return ret;

	memcpy(mac->value, mac_save, ARRAY_SIZE(mac_save));

	return 0;
}

static char *__state_mac_get(struct state_variable *var)
{
	struct state_mac *mac = to_state_mac(var);
	char *str;
	int ret;

	ret = asprintf(&str, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac->value[0], mac->value[1], mac->value[2],
			mac->value[3], mac->value[4], mac->value[5]);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	return str;
}

static int __state_string_set(struct state_variable *sv, const char *val)
{
	struct state_string *string = to_state_string(sv);
	int ret;

	ret = state_string_copy_to_raw(string, val);
	if (ret)
		return ret;
	free(string->value);
	string->value = xstrdup(val);

	return 0;
}

static char *__state_string_get(struct state_variable *var)
{
	struct state_string *string = to_state_string(var);
	char *str;

	if (string->raw[0])
		str = strndup(string->raw, string->var.size);
	else
		str = strdup("");

	if (!str)
		return ERR_PTR(-ENOMEM);

	return str;
}

char *state_get_var(struct state *state, const char *var)
{
	struct state_variable *sv;
	struct variable_str_type *vtype;

	sv = state_find_var(state, var);
	if (IS_ERR(sv))
		return NULL;

	vtype = state_find_type(sv->type->type);
	if (!vtype)
		return NULL;

	return vtype->get(sv);
}

static int state_set_var(struct state *state, const char *var, const char *val)
{
	struct state_variable *sv;
	struct variable_str_type *vtype;
	char *oldval;
	int ret;

	sv = state_find_var(state, var);
	if (IS_ERR(sv))
		return PTR_ERR(sv);

	vtype = state_find_type(sv->type->type);
	if (!vtype)
		return -ENODEV;

	if (!vtype->set)
		return -EPERM;

	oldval = vtype->get(sv);
	if (!IS_ERR(oldval)) {
		bool equal = strcmp(oldval, val) == 0;
		free(oldval);
		if (equal)
			return 0;
	}

	ret = vtype->set(sv, val);
	if (ret)
		return ret;

	state->dirty = 1;

	return 0;
}


struct state *state_get(const char *name, const char *filename, bool readonly, bool auth)
{
	struct device_node *root, *node;
	struct state *state;
	int ret;

	if (filename) {
		void *fdt;

		fdt = read_file(filename, NULL);
		if (!fdt) {
			pr_err("Unable to read devicetree file '%s'\n",
			       filename);
			return ERR_PTR(-ENOENT);
		}

		root = of_unflatten_dtb(fdt);
		free(fdt);
		if (IS_ERR(root)) {
			pr_err("Unable to read devicetree. %s\n",
			       strerror(-PTR_ERR(root)));
			return ERR_CAST(root);
		}
	} else {
		root = of_read_proc_devicetree();

		/* No device-tree in procfs / sysfs, try dtb file in the ESP */
		if (-PTR_ERR(root) == ENOENT) {
			const char *paths[] = {
				/* default mount paths used by systemd */
				"/boot/EFI/BAREBOX/state.dtb",
				"/boot/efi/EFI/BAREBOX/state.dtb",
				"/efi/EFI/BAREBOX/state.dtb",
				NULL
			};
			void *fdt;
			int i;

			for (i = 0; paths[i]; ++i) {
				fdt = read_file(paths[i], NULL);
				if (fdt)
					break;
			}
			if (fdt) {
				root = of_unflatten_dtb(fdt);
				free(fdt);
			}
			else
				root = ERR_PTR(-ENOENT);
		}
		if (IS_ERR(root)) {
			pr_err("Unable to read devicetree. %s\n",
			       strerror(-PTR_ERR(root)));
			return ERR_CAST(root);
		}
	}

	of_set_root_node(root);

	if (name) {
		node = of_find_node_by_path_or_alias(root, name);
		if (!node) {
			pr_err("no such node: %s\n", name);
			return ERR_PTR(-ENOENT);
		}
	} else {
		node = of_find_node_by_path_or_alias(root, "state");
		if (!node)
			node = of_find_node_by_path_or_alias(root, "/state");
		if (!node) {
			pr_err("Neither /aliases/state nor /state found\n");
			return ERR_PTR(-ENOENT);
		}
	}

	pr_debug("found state node %s:\n", node->full_name);
	if (pr_level_get() > 6)
		of_print_nodes(node, 0);

	state = state_new_from_node(node, readonly);
	if (IS_ERR(state)) {
		pr_err("unable to initialize state: %s\n",
				strerror(-PTR_ERR(state)));
		return ERR_CAST(state);
	}

	if (auth)
		ret = state_load(state);
	else
		ret = state_load_no_auth(state);

	if (ret)
		pr_err("Failed to load persistent state, continuing with defaults, %d\n", ret);

	return state;
}

enum opt {
	OPT_DUMP_SHELL = UCHAR_MAX + 1,
	OPT_VERSION    = UCHAR_MAX + 2,
};

static struct option long_options[] = {
	{"get",		required_argument,	0,	'g' },
	{"set",		required_argument,	0,	's' },
	{"name",	required_argument,	0,	'n' },
	{"input",	required_argument,	0,	'i' },
	{"dump",	no_argument,		0,	'd' },
	{"dump-shell",	no_argument,		0,	OPT_DUMP_SHELL },
	{"force",	no_argument,		0,	'f' },
	{"verbose",	no_argument,		0,	'v' },
	{"quiet",	no_argument,		0,	'q' },
	{"version",	no_argument,		0,	OPT_VERSION },
	{"help",	no_argument,		0,	'h' },
	{ }
};

static void usage(char *name)
{
	printf(
"Usage: %s [OPTIONS]\n"
"\n"
"-g, --get <variable>                      get the value of a variable\n"
"-s, --set <variable>=<value>              set the value of a variable\n"
"-n, --name <name>                         specify the state to use (default=\"state\"). Multiple states are allowed.\n"
"-i, --input <name>                        load the devicetree from a file instead of using the system devicetree.\n"
"-d, --dump                                dump the state\n"
"--dump-shell                              dump the state suitable for shell sourcing\n"
"-f, --force                               do not check for state manipulation via the HMAC\n"
"-v, --verbose                             increase verbosity\n"
"-q, --quiet                               decrease verbosity\n"
"--version                                 display version\n"
"-h, --help                                this help\n",
	name);
}

#define state_for_each_var(state, var) \
	list_for_each_entry(var, &(state)->variables, list)

struct state_set_get {
	char *arg;
	int get;
	struct list_head list;
};

struct state_list {
	const char *name;
	struct state *state;
	struct list_head list;
};

int main(int argc, char *argv[])
{
	struct state_variable *v;
	int ret, c, option_index;
	int do_dump = 0, do_dump_shell = 0;
	struct state_set_get *sg;
	struct list_head sg_list;
	struct state_list state_list;
	struct state_list *state;
	int lock_fd;
	int nr_states = 0;
	bool readonly = true;
	int pr_level = 5;
	int auth = 1;
	const char *dtb = NULL;

	INIT_LIST_HEAD(&sg_list);
	INIT_LIST_HEAD(&state_list.list);

	while (1) {
		c = getopt_long(argc, argv, "hg:s:i:dvn:qf", long_options, &option_index);
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage(argv[0]);
			exit(0);
		case OPT_VERSION:
			printf(PACKAGE_STRING "\n");
			printf("Configured with build-time option '--%s-state-backward-compatibility'.\n",
			       (CONFIG_STATE_BACKWARD_COMPATIBLE) ? "enable" : "disable");
			printf("                                  '--%s-lock-device-node'.\n",
			       (CONFIG_LOCK_DEVICE_NODE) ? "enable" : "disable");
			exit(0);
		case 'g':
			sg = xzalloc(sizeof(*sg));
			sg->get = 1;
			sg->arg = optarg;
			list_add_tail(&sg->list, &sg_list);
			break;
		case 's':
			sg = xzalloc(sizeof(*sg));
			sg->get = 0;
			sg->arg = optarg;
			list_add_tail(&sg->list, &sg_list);
			readonly = false;
			break;
		case 'f':
			auth = 0;
			break;
		case 'd':
			do_dump = 1;
			break;
		case OPT_DUMP_SHELL:
			do_dump_shell = 1;
			break;
		case 'v':
			pr_level++;
			break;
		case 'q':
			pr_level--;
			break;
		case 'n':
		{
			struct state_list *name;

			name = xzalloc(sizeof(*name));
			name->name = optarg;

			list_add_tail(&name->list, &state_list.list);
			++nr_states;
			break;
		}
		case 'i':
			dtb = optarg;
			break;
		case ':':
		case '?':
		default:
			exit(1);
			break;

		}
	}

	if (optind < argc) {
		pr_err("Invalid argument: %s\n", argv[optind]);
		exit(1);
	}

	pr_level_set(pr_level);

	if (nr_states == 0) {
		struct state_list *new_state;

		new_state = xzalloc(sizeof(*new_state));
		new_state->name = NULL;

		list_add_tail(&new_state->list, &state_list.list);
		++nr_states;
	}

	if (!IS_ENABLED(CONFIG_LOCK_DEVICE_NODE)) {
		lock_fd = open(BAREBOX_STATE_LOCKFILE, O_CREAT | O_RDWR, 0600);
		if (lock_fd < 0) {
			pr_err("Failed to open lock-file " BAREBOX_STATE_LOCKFILE "\n");
			exit(1);
		}

		ret = flock(lock_fd, LOCK_EX);
		if (ret < 0) {
			pr_err("Failed to lock " BAREBOX_STATE_LOCKFILE ": %m\n");
			close(lock_fd);
			exit(1);
		}
	}

	list_for_each_entry(state, &state_list.list, list) {
		state->state = state_get(state->name, dtb, readonly, auth);
		if (!IS_ERR(state->state) && !state->name)
			state->name = state->state->name;
		if (IS_ERR(state->state)) {
			ret = 1;
			goto out_unlock;
		}
	}

	if (do_dump) {
		list_for_each_entry(state, &state_list.list, list) {
			state_for_each_var(state->state, v) {
				struct variable_str_type *vtype;
				vtype = state_find_type(v->type->type);

				if (!vtype) {
					pr_err("no such type: %d\n", v->type->type);
					ret = 1;
					goto out_unlock;
				}

				if (nr_states > 1)
					printf("%s.%s=%s", state->name, v->name,
					       vtype->get(v));
				else
					printf("%s=%s", v->name, vtype->get(v));
				if (pr_level_get() > 5) {
					printf(", type=%s", vtype->type_name);
					if (vtype->info)
						vtype->info(v);
				}
				printf("\n");
			}
		}
	}

	if (do_dump_shell) {
		list_for_each_entry(state, &state_list.list, list) {
			state_for_each_var(state->state, v) {
				struct variable_str_type *vtype;
				char *name, *ptr;

				/* replace "." by "_" to make it var name shell compatible */
				name = strdup(v->name);
				ptr = name;
				while ((ptr = strchr(ptr, '.')))
					*ptr++ = '_';

				vtype = state_find_type(v->type->type);
				printf("%s_%s=\"%s\"\n", state->name, name, vtype->get(v));
			}
		}
	}

	list_for_each_entry(sg, &sg_list, list) {
		char *arg = sg->arg;
		char *statename_end = strchr(sg->arg, '.');
		int statename_len;
		state = &state_list;

		if (statename_end) {
			statename_len = statename_end - sg->arg;

			list_for_each_entry(state, &state_list.list, list) {
				if (strlen(state->name) == statename_len &&
				    !strncmp(state->name, sg->arg, statename_len)) {
					arg = statename_end + 1;
					break;
				}
			}
		}
		if (state == &state_list) {
			state = list_first_entry(&state_list.list, struct state_list, list);
		}
		if (sg->get) {
			char *val = state_get_var(state->state, arg);
			if (!val) {
				pr_err("no such variable: %s\n", arg);
				ret = 1;
				goto out_unlock;
			}
			printf("%s\n", val);

		} else {
			char *var, *val;

			var = arg;
			val = index(arg, '=');
			if (!val) {
				pr_err("usage: -s var=val\n");
				ret = 1;
				goto out_unlock;
			}
			*val++ = '\0';
			ret = state_set_var(state->state, var, val);
			if (ret) {
				pr_err("Failed to set variable %s in state %s to %s: %s\n",
						var, state->name, val,
						strerror(-ret));
				ret = 1;
				goto out_unlock;
			}
		}
	}

	list_for_each_entry(state, &state_list.list, list) {
		if (state->state->dirty) {
			ret = state_save(state->state);
			if (ret) {
				pr_err("Failed to save state: %s\n", strerror(-ret));
				ret = 1;
				goto out_unlock;
			}
		}
	}

	ret = 0;
out_unlock:
	if (!IS_ENABLED(CONFIG_LOCK_DEVICE_NODE)) {
		flock(lock_fd, LOCK_UN);
		close(lock_fd);
	}

	return ret;
}
