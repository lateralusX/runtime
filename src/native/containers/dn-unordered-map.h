
#ifndef __DN_UNORDERED_MAP_H__
#define __DN_UNORDERED_MAP_H__

#include "dn-unordered-map.h"
#include "dn-list.h"
#include "dn-utils.h"

typedef struct _dn_unordered_map_slot_t dn_unordered_map_slot_t;

struct _dn_unordered_map_slot_t {
	void *key;
	void *value;
	dn_unordered_map_slot_t *next;
};

typedef struct _dn_unordered_map_t dn_unordered_map_t;

struct _dn_unordered_map_t {
	dn_hash_func_t hash_func;
	dn_equal_func_t key_equal_func;

	dn_unordered_map_slot_t **table;
	int32_t table_size;
	int32_t in_use;
	int32_t threshold;
	int32_t last_rehash;
	dn_destory_notify_func_t value_destroy_func, key_destroy_func;
};

typedef struct _dn_unordered_map_iter_t dn_unordered_map_iter_t;

struct _dn_unordered_map_iter_t
{
	dn_unordered_map_t *map;
	int32_t index;
	dn_unordered_map_slot_t *slot;
};

dn_unordered_map_t *
dn_unordered_map_new (
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func);

dn_unordered_map_t *
dn_unordered_map_new_full (
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_destory_notify_func_t key_destroy_func,
	dn_destory_notify_func_t value_destroy_func);

void
dn_unordered_map_insert_replace (
	dn_unordered_map_t *map,
	void *key,
	void *value,
	bool replace);

uint32_t
dn_unordered_map_size (dn_unordered_map_t *map);

dn_list_t *
dn_unordered_map_get_keys (dn_unordered_map_t *map);

dn_list_t *
dn_unordered_map_get_values (dn_unordered_map_t *map);

bool
dn_unordered_map_contains (
	dn_unordered_map_t *map,
	const void *key);

void *
dn_unordered_map_lookup (
	dn_unordered_map_t *map,
	const void *key);

bool
dn_unordered_map_lookup_extended (
	dn_unordered_map_t *map,
	const void *key,
	void **orig_key,
	void **value);

void
dn_unordered_map_foreach (
	dn_unordered_map_t *map,
	dn_h_func_t func,
	void *user_data);

void *
dn_unordered_map_find (
	dn_unordered_map_t *map,
	dn_hr_func_t predicate,
	void *user_data);

bool
dn_unordered_map_remove (
	dn_unordered_map_t *map,
	const void *key);

bool
dn_unordered_map_steal (
	dn_unordered_map_t *map,
	const void *key);

void
dn_unordered_map_remove_all (dn_unordered_map_t *map);

uint32_t
dn_unordered_map_foreach_remove (
	dn_unordered_map_t *map,
	dn_hr_func_t func,
	void *user_data);

uint32_t
dn_unordered_map_foreach_steal (
	dn_unordered_map_t *map,
	dn_hr_func_t func,
	void *user_data);

void
dn_unordered_map_destroy (dn_unordered_map_t *map);

void
dn_unordered_map_print_stats (dn_unordered_map_t *map);

void
dn_unordered_map_iter_init (
	dn_unordered_map_iter_t *iter,
	dn_unordered_map_t *map);

bool
dn_unordered_map_iter_next (
	dn_unordered_map_iter_t *iter,
	void **key,
	void **value);

static inline void
dn_unordered_map_insert (
	dn_unordered_map_t *map,
	void *key,
	void *value)
{
	dn_unordered_map_insert_replace (map, key, value, false);
}

static inline void
dn_unordered_map_replace (
	dn_unordered_map_t *map,
	void *key,
	void *value)
{
	dn_unordered_map_insert_replace (map, key, value, true);
}

static inline void
dn_unordered_map_add (
	dn_unordered_map_t *map,
	void *key,
	void *value)
{
	dn_unordered_map_insert_replace (map, key, value, true);
}

bool
dn_direct_equal (const void *v1, const void *v2);

uint32_t
dn_direct_hash (const void *v1);

bool
dn_int_equal (const void *v1, const void *v2);

uint32_t
dn_int_hash (const void *v1);

bool
dn_str_equal (const void *v1, const void *v2);

uint32_t
dn_str_hash (const void *v1);

#endif /* __DN_UNORDERED_MAP_H__ */
