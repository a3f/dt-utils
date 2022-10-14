#ifndef __DT_DT_H
#define __DT_DT_H

#include <stdint.h>
#include <dt/list.h>
#include <dt/common.h>
#include <asm/byteorder.h>

/* Default string compare functions */
#define of_compat_cmp(s1, s2, l)	strcasecmp((s1), (s2))
#define of_prop_cmp(s1, s2)		strcmp((s1), (s2))
#define of_node_cmp(s1, s2)		strcasecmp((s1), (s2))

#define OF_BAD_ADDR      ((uint64_t)-1)

typedef uint32_t phandle;

struct property {
	char *name;
	int length;
	void *value;
	struct list_head list;
};

struct device_node {
	char *name;
	char *full_name;

	struct list_head properties;
	struct device_node *parent;
	struct list_head children;
	struct list_head parent_list;
	struct list_head list;
	phandle phandle;
};

struct of_device_id {
	char *compatible;
	unsigned long data;
};

#define MAX_PHANDLE_ARGS 8
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	uint32_t args[MAX_PHANDLE_ARGS];
};

#define OF_MAX_RESERVE_MAP	16
struct of_reserve_map {
	uint64_t start[OF_MAX_RESERVE_MAP];
	uint64_t end[OF_MAX_RESERVE_MAP];
	int num_entries;
};

int of_add_reserve_entry(uint64_t start, uint64_t end);
struct of_reserve_map *of_get_reserve_map(void);
void of_clean_reserve_map(void);
void fdt_add_reserve_map(void *fdt);

struct device_d;
struct driver_d;

int of_match(struct device_d *dev, struct driver_d *drv);

struct fdt_header *fdt_get_tree(void);

struct fdt_header *of_get_fixed_tree(struct device_node *node);

/* Helper to read a big number; size is in cells (not bytes) */
static inline uint64_t of_read_number(const __be32 *cell, int size)
{
	uint64_t r = 0;
	while (size--)
		r = (r << 32) | __be32_to_cpu(*(cell++));
	return r;
}

/* Helper to write a big number; size is in cells (not bytes) */
static inline void of_write_number(void *__cell, uint64_t val, int size)
{
	__be32 *cell = __cell;

	while (size--) {
		cell[size] = __cpu_to_be32(val);
		val >>= 32;
	}
}

void of_print_property(const void *data, int len);
void of_print_cmdline(struct device_node *root);

void of_print_nodes(struct device_node *node, int indent);
int of_probe(void);
int of_parse_dtb(struct fdt_header *fdt);
struct device_node *of_unflatten_dtb(const void *fdt);

struct cdev;

extern int of_n_addr_cells(struct device_node *np);
extern int of_n_size_cells(struct device_node *np);

extern struct property *of_find_property(const struct device_node *np,
					const char *name, int *lenp);
extern const void *of_get_property(const struct device_node *np,
				const char *name, int *lenp);

extern int of_set_property(struct device_node *node, const char *p,
			const void *val, int len, int create);
extern struct property *of_new_property(struct device_node *node,
				const char *name, const void *data, int len);
extern void of_delete_property(struct property *pp);

extern struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name);
extern struct device_node *of_find_node_by_path_from(struct device_node *from,
						const char *path);
extern struct device_node *of_find_node_by_path(const char *path);
extern struct device_node *of_find_node_by_phandle(phandle phandle);
extern struct device_node *of_find_node_by_type(struct device_node *from,
	const char *type);
extern struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compat);
extern struct udev_device *of_find_device_by_node_path(const char *of_full_path);
extern const struct of_device_id *of_match_node(
	const struct of_device_id *matches, const struct device_node *node);
extern struct device_node *of_find_matching_node_and_match(
	struct device_node *from,
	const struct of_device_id *matches,
	const struct of_device_id **match);
extern struct device_node *of_find_node_with_property(
	struct device_node *from, const char *prop_name);

extern struct device_node *of_new_node(struct device_node *parent,
				const char *name);
extern struct device_node *of_create_node(struct device_node *root,
					const char *path);
extern void of_delete_node(struct device_node *node);

extern int of_machine_is_compatible(const char *compat);
extern int of_device_is_compatible(const struct device_node *device,
		const char *compat);
extern int of_device_is_available(const struct device_node *device);

extern struct device_node *of_get_parent(const struct device_node *node);
extern struct device_node *of_get_next_available_child(
	const struct device_node *node, struct device_node *prev);
extern int of_get_child_count(const struct device_node *parent);
extern int of_get_available_child_count(const struct device_node *parent);
extern struct device_node *of_get_child_by_name(const struct device_node *node,
					const char *name);

extern int of_property_read_u32_index(const struct device_node *np,
				       const char *propname,
				       uint32_t index, uint32_t *out_value);
extern int of_property_read_u8_array(const struct device_node *np,
			const char *propname, uint8_t *out_values, size_t sz);
extern int of_property_read_u16_array(const struct device_node *np,
			const char *propname, uint16_t *out_values, size_t sz);
extern int of_property_read_u32_array(const struct device_node *np,
				      const char *propname,
				      uint32_t *out_values,
				      size_t sz);
extern int of_property_read_u64(const struct device_node *np,
				const char *propname, uint64_t *out_value);

extern int of_property_read_string(struct device_node *np,
				   const char *propname,
				   const char **out_string);
extern int of_property_read_string_index(struct device_node *np,
					 const char *propname,
					 int index, const char **output);
extern int of_property_match_string(struct device_node *np,
				    const char *propname,
				    const char *string);
extern int of_property_count_strings(struct device_node *np,
				     const char *propname);

extern const __be32 *of_prop_next_u32(struct property *prop,
				const __be32 *cur, uint32_t *pu);
extern const char *of_prop_next_string(struct property *prop, const char *cur);

extern int of_property_write_bool(struct device_node *np,
				const char *propname, const bool value);
extern int of_property_write_u8_array(struct device_node *np,
				const char *propname, const uint8_t *values,
				size_t sz);
extern int of_property_write_u16_array(struct device_node *np,
				const char *propname, const uint16_t *values,
				size_t sz);
extern int of_property_write_u32_array(struct device_node *np,
				const char *propname, const uint32_t *values,
				size_t sz);
extern int of_property_write_u64_array(struct device_node *np,
				const char *propname, const uint64_t *values,
				size_t sz);
extern int of_property_write_string(struct device_node *np,
				const char *propname, const char *value);
extern int of_property_write_strings(struct device_node *np,
		const char *propname, ...) __attribute__((__sentinel__));

extern struct device_node *of_parse_phandle(const struct device_node *np,
					    const char *phandle_name,
					    int index);
extern int of_parse_phandle_with_args(const struct device_node *np,
	const char *list_name, const char *cells_name, int index,
	struct of_phandle_args *out_args);
extern int of_count_phandle_with_args(const struct device_node *np,
	const char *list_name, const char *cells_name);

extern void of_alias_scan(void);
extern int of_alias_get_id(struct device_node *np, const char *stem);
extern const char *of_alias_get(struct device_node *np);
extern int of_modalias_node(struct device_node *node, char *modalias, int len);

extern struct device_node *of_get_root_node(void);
extern int of_set_root_node(struct device_node *node);

extern int of_platform_populate(struct device_node *root,
				const struct of_device_id *matches,
				struct device_d *parent);
extern struct device_d *of_find_device_by_node(struct device_node *np);

int of_device_is_stdout_path(struct device_d *dev);
const char *of_get_model(void);
void *of_flatten_dtb(struct device_node *node);
int of_add_memory(struct device_node *node, bool dump);
void of_add_memory_bank(struct device_node *node, bool dump, int r,
		uint64_t base, uint64_t size);

int of_get_devicepath(struct device_node *partition_node, char **devnode, off_t *offset,
		size_t *size);

#define for_each_node_by_name(dn, name) \
	for (dn = of_find_node_by_name(NULL, name); dn; \
	     dn = of_find_node_by_name(dn, name))
#define for_each_compatible_node(dn, type, compatible) \
	for (dn = of_find_compatible_node(NULL, type, compatible); dn; \
	     dn = of_find_compatible_node(dn, type, compatible))
static inline struct device_node *of_find_matching_node(
	struct device_node *from,
	const struct of_device_id *matches)
{
	return of_find_matching_node_and_match(from, matches, NULL);
}
#define for_each_matching_node(dn, matches) \
	for (dn = of_find_matching_node(NULL, matches); dn; \
	     dn = of_find_matching_node(dn, matches))
#define for_each_matching_node_and_match(dn, matches, match) \
	for (dn = of_find_matching_node_and_match(NULL, matches, match); \
	     dn; dn = of_find_matching_node_and_match(dn, matches, match))
#define for_each_node_with_property(dn, prop_name) \
	for (dn = of_find_node_with_property(NULL, prop_name); dn; \
	     dn = of_find_node_with_property(dn, prop_name))

#define for_each_child_of_node(parent, child) \
	list_for_each_entry(child, &parent->children, parent_list)
#define for_each_available_child_of_node(parent, child) \
	for (child = of_get_next_available_child(parent, NULL); child != NULL; \
	     child = of_get_next_available_child(parent, child))

/**
 * of_property_read_bool - Findfrom a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 *
 * Search for a property in a device node.
 * Returns true if the property exist false otherwise.
 */
static inline bool of_property_read_bool(const struct device_node *np,
					 const char *propname)
{
	struct property *prop = of_find_property(np, propname, NULL);

	return prop ? true : false;
}

static inline int of_property_read_u8(const struct device_node *np,
				       const char *propname,
				       uint8_t *out_value)
{
	return of_property_read_u8_array(np, propname, out_value, 1);
}

static inline int of_property_read_u16(const struct device_node *np,
				       const char *propname,
				       uint16_t *out_value)
{
	return of_property_read_u16_array(np, propname, out_value, 1);
}

static inline int of_property_read_u32(const struct device_node *np,
				       const char *propname,
				       uint32_t *out_value)
{
	return of_property_read_u32_array(np, propname, out_value, 1);
}

/*
 * struct property *prop;
 * const __be32 *p;
 * uint32_t u;
 *
 * of_property_for_each_u32(np, "propname", prop, p, u)
 *         printk("U32 value: %x\n", u);
 */
#define of_property_for_each_u32(np, propname, prop, p, u)	\
	for (prop = of_find_property(np, propname, NULL),	\
		p = of_prop_next_u32(prop, NULL, &u);		\
		p;						\
		p = of_prop_next_u32(prop, p, &u))

/*
 * struct property *prop;
 * const char *s;
 *
 * of_property_for_each_string(np, "propname", prop, s)
 *         printk("String value: %s\n", s);
 */
#define of_property_for_each_string(np, propname, prop, s)	\
	for (prop = of_find_property(np, propname, NULL),	\
		s = of_prop_next_string(prop, NULL);		\
		s;						\
		s = of_prop_next_string(prop, s))

static inline int of_property_write_u8(struct device_node *np,
				       const char *propname, uint8_t value)
{
	return of_property_write_u8_array(np, propname, &value, 1);
}

static inline int of_property_write_u16(struct device_node *np,
					const char *propname, uint16_t value)
{
	return of_property_write_u16_array(np, propname, &value, 1);
}

static inline int of_property_write_u32(struct device_node *np,
					const char *propname,
					uint32_t value)
{
	return of_property_write_u32_array(np, propname, &value, 1);
}

static inline int of_property_write_u64(struct device_node *np,
					const char *propname,
					uint64_t value)
{
	return of_property_write_u64_array(np, propname, &value, 1);
}

extern const struct of_device_id of_default_bus_match_table[];

int of_device_enable(struct device_node *node);
int of_device_enable_path(const char *path);
int of_device_disable(struct device_node *node);
int of_device_disable_path(const char *path);

phandle of_get_tree_max_phandle(struct device_node *root);
phandle of_node_create_phandle(struct device_node *node);
struct device_node *of_find_node_by_alias(struct device_node *root,
		const char *alias);
struct device_node *of_find_node_by_path_or_alias(struct device_node *root,
		const char *str);

static inline struct device_node *of_find_root_node(struct device_node *node)
{
	while (node->parent)
		node = node->parent;

	return node;
}

struct device_node *of_read_proc_devicetree(void);

static inline struct device_node *of_find_node_by_reproducible_name(struct device_node *from,
								    const char *name)
{
	return NULL;
}

static inline char *of_get_reproducible_name(struct device_node *node)
{
	return NULL;
}

#endif /* __DT_DT_H */
