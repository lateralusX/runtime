
#ifndef __DN_UNORDERED_MAP_EX_H__
#define __DN_UNORDERED_MAP_EX_H__

#include "dn-utils.h"
#include "dn-unordered-map.h"

#define DN_UNORDERED_MAP_EX_FOREACH_BEGIN(map,key_type,key_name,value_type,value_name) do { \
	key_type key_name; \
	value_type value_name; \
	void * _value##key_name; \
	dn_unordered_map_iter_t __it##key_name; \
	dn_unordered_map_iter_init (&__it##key_name, map); \
	while (dn_unordered_map_iter_next (&__it##key_name, (void **)&key_name, (void **)&_value##key_name)) { \
		value_name = ((value_type)(uintptr_t)_value##key_name);

#define DN_UNORDERED_MAP_EX_FOREACH_KEY_BEGIN(map,key_type,key_name) do { \
	key_type key_name; \
	dn_unordered_map_iter_t __it##key_name; \
	dn_unordered_map_iter_init (&__it##key_name, map); \
	while (dn_unordered_map_iter_next (&__it##key_name, (void **)&key_name, NULL)) {

#define DN_UNORDERED_MAP_EX_FOREACH_END \
		} \
	} while (0)

static inline dn_unordered_map_t *
dn_unordered_map_ex_alloc (
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_destory_notify_func_t key_destroy_func,
	dn_destory_notify_func_t value_destroy_func)
{
	return dn_unordered_map_new_full (hash_func, equal_func, key_destroy_func, value_destroy_func);
}

static inline void
dn_unordered_map_ex_free (dn_unordered_map_t **map)
{
	dn_unordered_map_destroy (*map);
	*map = NULL;
}

static inline bool
dn_unordered_map_ex_insert (
	dn_unordered_map_t *map,
	void *key,
	void *value)
{
	// Should be combined into the insert function.
	if (dn_unordered_map_contains (map, key))
		return false;

	dn_unordered_map_insert (map, key, value);
	return true;
}

static inline void
dn_unordered_map_ex_clear (dn_unordered_map_t *map)
{
	dn_unordered_map_remove_all (map);
}

static inline bool
dn_unordered_map_ex_find (
	dn_unordered_map_t *map,
	const void *key,
	void **value)
{
	return dn_unordered_map_lookup_extended (map, key, NULL, value);
}

static inline uint32_t
dn_unordered_map_ex_size (dn_unordered_map_t *map)
{
	return dn_unordered_map_size (map);
}

static inline bool
dn_unordered_map_ex_insert_or_assign (
	dn_unordered_map_t *map,
	void *key,
	void *value)
{
	// Should return true on insert and false on assign.
	dn_unordered_map_replace (map, key, value);
	return true;
}

static inline bool
dn_unordered_map_ex_erase (
	dn_unordered_map_t *map,
	void *key)
{
	return dn_unordered_map_remove (map, key);
}

#define DN_UNORDERED_MAP_EX_VALUE_TYPE(value_type) \
_DN_STATIC_ASSERT (sizeof(value_type) <= sizeof(uintptr_t)); \
static inline bool \
dn_unordered_map_ex_insert_##value_type (dn_unordered_map_t *map, void *key, value_type value) \
{ \
	return dn_unordered_map_ex_insert (map, key, ((void *)(uintptr_t)value)); \
} \
static inline bool \
dn_unordered_map_ex_find_##value_type (dn_unordered_map_t *map, void *key, value_type *value) \
{ \
	void *_value = NULL; \
	bool result = dn_unordered_map_ex_find (map, key, &_value); \
	*value = ((value_type)(uintptr_t)_value); \
	return result; \
} \
static inline bool \
dn_unordered_map_ex_insert_or_assign_##value_type (dn_unordered_map_t *map, void *key, value_type value) \
{ \
	return dn_unordered_map_ex_insert_or_assign (map, key, ((void *)(uintptr_t)value)); \
}

DN_UNORDERED_MAP_EX_VALUE_TYPE (bool)
DN_UNORDERED_MAP_EX_VALUE_TYPE (int8_t)
DN_UNORDERED_MAP_EX_VALUE_TYPE (uint8_t)
DN_UNORDERED_MAP_EX_VALUE_TYPE (int16_t)
DN_UNORDERED_MAP_EX_VALUE_TYPE (uint16_t)
DN_UNORDERED_MAP_EX_VALUE_TYPE (int32_t)
DN_UNORDERED_MAP_EX_VALUE_TYPE (uint32_t)

#endif /* __DN_QUEUE_EX_H__ */
