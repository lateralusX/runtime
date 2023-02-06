
#ifndef __DN_UMAP_EX_H__
#define __DN_UMAP_EX_H__

#include "dn-utils.h"
#include "dn-allocator.h"

typedef struct _dn_umap_node_t dn_umap_node_t;
struct _dn_umap_node_t {
	void *key;
	void *value;
	dn_umap_node_t *next;
};

typedef struct _dn_umap_t dn_umap_t;
struct _dn_umap_t {
	struct {
		dn_umap_node_t **_buckets;
		dn_hash_func_t _hash_func;
		dn_equal_func_t _key_equal_func;
		dn_func_t _value_dispose_func;
		dn_func_t _key_dispose_func;
		dn_allocator_t *_allocator;
		uint32_t _bucket_count;
		uint32_t _node_count;
		uint32_t _threshold;
		uint32_t _last_rehash;
	} _internal;
};

typedef struct _dn_umap_it_t dn_umap_it_t;
struct _dn_umap_it_t
{
	struct {
		dn_umap_t *_map;
		dn_umap_node_t *_node;
		uint32_t _index;
	} _internal;
};

typedef struct _dn_umap_result_t dn_umap_result_t;
struct _dn_umap_result_t {
	bool result;
	dn_umap_it_t it;
};

dn_umap_it_t
_dn_umap_begin (dn_umap_t *map);

bool
_dn_umap_it_next (dn_umap_it_t *it);

static inline void
dn_umap_it_advance (
	dn_umap_it_t *it,
	uint32_t n)
{
	while (n && _dn_umap_it_next (it))
		n--;
}

static inline dn_umap_it_t
dn_umap_it_next (dn_umap_it_t it)
{
	_dn_umap_it_next (&it);
	return it;
}

static inline void *
dn_umap_it_key (dn_umap_it_t it)
{
	return it._internal._node->key;
}

#define dn_umap_it_key_t(it, type) \
	(type)(uintptr_t)(dn_umap_it_key (it))

static inline void *
dn_umap_it_value (dn_umap_it_t it)
{
	return it._internal._node->value;
}

#define dn_umap_it_value_t(it, type) \
	(type)(uintptr_t)(dn_umap_it_value ((it)))

static inline bool
dn_umap_it_begin (dn_umap_it_t it)
{
	dn_umap_it_t begin = _dn_umap_begin (it._internal._map);
	return (begin._internal._node == it._internal._node && begin._internal._index == it._internal._index);
}

static inline bool
dn_umap_it_end (dn_umap_it_t it)
{
	return !(it._internal._node);
}

#define DN_UMAP_FOREACH_BEGIN(map,key_type,key_name,value_type,value_name) do { \
	key_type key_name; \
	value_type value_name; \
	for (dn_umap_it_t __it##key_name = dn_umap_begin (map); !dn_umap_it_end (__it##key_name); __it##key_name = dn_umap_it_next (__it##key_name)) { \
		key_name = ((key_type)(uintptr_t)dn_umap_it_key (__it##key_name)); \
		value_name = ((value_type)(uintptr_t)dn_umap_it_value (__it##key_name));

#define DN_UMAP_FOREACH_KEY_BEGIN(map,key_type,key_name) do { \
	key_type key_name; \
	for (dn_umap_it_t __it##key_name = dn_umap_begin (map); !dn_umap_it_end (__it##key_name); __it##key_name = dn_umap_it_next (__it##key_name)) { \
		key_name = ((key_type)(uintptr_t)dn_umap_it_key (__it##key_name));

#define DN_UMAP_FOREACH_END \
		} \
	} while (0)

dn_umap_t *
_dn_umap_alloc (
	dn_allocator_t *allocator,
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_func_t key_dispose_func,
	dn_func_t value_dispose_func);

bool
_dn_umap_init (
	dn_umap_t *map,
	dn_allocator_t *allocator,
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_func_t key_dispose_func,
	dn_func_t value_dispose_func);

static inline dn_umap_t *
dn_umap_custom_alloc (
	dn_allocator_t *allocator,
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_func_t key_dispose_func,
	dn_func_t value_dispose_func)
{
	return _dn_umap_alloc (allocator, hash_func, equal_func, key_dispose_func, value_dispose_func);
}

static inline dn_umap_t *
dn_umap_alloc (void)
{
	return _dn_umap_alloc (DN_DEFAULT_ALLOCATOR, NULL, NULL, NULL, NULL);
}

void
dn_umap_free (dn_umap_t *map);

static inline bool
dn_umap_custom_init (
	dn_umap_t *map,
	dn_allocator_t *allocator,
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_func_t key_dispose_func,
	dn_func_t value_dispose_func)
{
	return _dn_umap_init (map, allocator, hash_func, equal_func, key_dispose_func, value_dispose_func);
}

static inline bool
dn_umap_init (dn_umap_t *map)
{
	return _dn_umap_init (map, DN_DEFAULT_ALLOCATOR, NULL, NULL, NULL, NULL);
}

void
dn_umap_dispose (dn_umap_t *map);

static inline dn_umap_it_t
dn_umap_begin (dn_umap_t *map)
{
	return _dn_umap_begin (map);
}

static inline dn_umap_it_t
dn_umap_end (dn_umap_t *map)
{
	dn_umap_it_t it = { { map, NULL, 0 } };
	return it;
}

static inline bool
dn_umap_empty (const dn_umap_t *map)
{
	return !map || map->_internal._node_count == 0;
}

static inline uint32_t
dn_umap_size (const dn_umap_t *map)
{
	if (!map)
		return 0;
	return map->_internal._node_count;
}

static inline uint32_t
dn_umap_max_size (const dn_umap_t *map)
{
	DN_UNREFERENCED_PARAMETER (map);
	return UINT32_MAX;
}

void
dn_umap_clear (dn_umap_t *map);

dn_umap_result_t
dn_umap_insert (
	dn_umap_t *map,
	void *key,
	void *value);

dn_umap_result_t
dn_umap_insert_or_assign (
	dn_umap_t *map,
	void *key,
	void *value);

dn_umap_it_t
dn_umap_erase (dn_umap_it_t position);

uint32_t
dn_umap_erase_key (
	dn_umap_t *map,
	const void *key);

bool
dn_umap_extract_key (
	dn_umap_t *map,
	const void *key,
	void **out_key,
	void **out_value);

dn_umap_it_t
dn_umap_custom_find (
	dn_umap_t *map,
	const void *key,
	dn_equal_func_t func);

static inline dn_umap_it_t
dn_umap_find (
	dn_umap_t *map,
	const void *key)
{
	return dn_umap_custom_find (map, key, NULL);
}

static inline bool
dn_umap_contains (
	dn_umap_t *map,
	const void *key)
{
	if (DN_UNLIKELY (!map))
		return false;

	dn_umap_it_t it = dn_umap_find (map, key);
	return !dn_umap_it_end (it);
}

void
dn_umap_for_each (
	dn_umap_t *map,
	dn_key_value_func_t func,
	void *data);

void
dn_umap_rehash (
	dn_umap_t *map,
	uint32_t count);

void
dn_umap_reserve (
	dn_umap_t *map,
	uint32_t count);

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

#define DN_UMAP_IT_KEY_T(key_type_name, key_type) \
_DN_STATIC_ASSERT(sizeof(key_type) <= sizeof(uintptr_t)); \
static inline key_type \
dn_umap_it_key_ ## key_type_name (dn_umap_it_t it) \
{ \
	return (key_type)(uintptr_t)(it._internal._node->key); \
}

#define DN_UMAP_IT_VALUE_T(value_type_name, value_type) \
_DN_STATIC_ASSERT(sizeof(value_type) <= sizeof(uintptr_t)); \
static inline value_type \
dn_umap_it_value_ ## value_type_name (dn_umap_it_t it) \
{ \
	return (value_type)(uintptr_t)(it._internal._node->value); \
}

#define DN_UMAP_T(key_type_name, key_type, value_type_name, value_type) \
_DN_STATIC_ASSERT(sizeof(key_type) <= sizeof(uintptr_t) && sizeof(value_type) <= sizeof(uintptr_t)); \
static inline dn_umap_result_t \
dn_umap_ ## key_type_name ## _ ## value_type_name ## _insert (dn_umap_t *map, key_type key, value_type value) \
{ \
	return dn_umap_insert (map, ((void *)(uintptr_t)key), ((void *)(uintptr_t)value)); \
} \
static inline dn_umap_result_t \
dn_umap_ ## key_type_name ## _ ## value_type_name ## _insert_or_assign (dn_umap_t *map, key_type key, value_type value) \
{ \
	return dn_umap_insert_or_assign (map, ((void *)(uintptr_t)key), ((void *)(uintptr_t)value)); \
} \
static inline dn_umap_it_t \
dn_umap_ ## key_type_name ## _ ## value_type_name ## _find (dn_umap_t *map, key_type key) \
{ \
	return dn_umap_find (map, ((const void *)(uintptr_t)key)); \
}

DN_UMAP_IT_KEY_T (ptr, void *)

DN_UMAP_IT_VALUE_T (bool, bool)
DN_UMAP_IT_VALUE_T (int8_t, int8_t)
DN_UMAP_IT_VALUE_T (uint8_t, uint8_t)
DN_UMAP_IT_VALUE_T (int16_t, int16_t)
DN_UMAP_IT_VALUE_T (uint16_t, uint16_t)
DN_UMAP_IT_VALUE_T (int32_t, int32_t)
DN_UMAP_IT_VALUE_T (uint32_t, uint32_t)

DN_UMAP_T (ptr, void *, bool, bool)
DN_UMAP_T (ptr, void *, int8, int8_t)
DN_UMAP_T (ptr, void *, uint8, uint8_t)
DN_UMAP_T (ptr, void *, int16, int16_t)
DN_UMAP_T (ptr, void *, uint16, uint16_t)
DN_UMAP_T (ptr, void *, int32, int32_t)
DN_UMAP_T (ptr, void *, uint32, uint32_t)

#endif /* __DN_UMAP_EX_H__ */
