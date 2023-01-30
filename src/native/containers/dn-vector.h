// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#ifndef __DN_VECTOR_H__
#define __DN_VECTOR_H__

#include "dn-utils.h"
#include "dn-allocator.h"

#define DN_DEFINE_VECTOR_T_STRUCT(name, type) \
typedef struct _ ## name ## _t name ## _t; \
struct _ ## name ## _t { \
	type* data; \
	uint32_t size; \
	uint32_t version; \
	struct { \
		dn_allocator_t *_allocator; \
		uint32_t _element_size; \
		uint32_t _capacity; \
	}_internal; \
}; \
static inline bool name ## _check_version (name ## _t *vector) { return vector->version == sizeof (name ## _t); }

DN_DEFINE_VECTOR_T_STRUCT(dn_vector, uint8_t);

#define DN_VECTOR_FOREACH_BEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	DN_ASSERT (dn_vector_check_version (vector)); \
	DN_ASSERT (sizeof (var_type) == vector->_internal._element_size); \
	for (uint32_t __i_ ## var_name = 0; __i_ ## var_name < (vector)->size; ++__i_ ## var_name) { \
		var_name = *dn_vector_index_t (vector, var_type, __i_##var_name);

#define DN_VECTOR_FOREACH_RBEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	DN_ASSERT (dn_vector_check_version (vector)); \
	DN_ASSERT (sizeof (var_type) == vector->_internal._element_size); \
	for (uint32_t __i_ ## var_name = (vector)->size; __i_ ## var_name > 0; --__i_ ## var_name) { \
		var_name = *dn_vector_index_t (vector, var_type, __i_ ## var_name - 1);

#define DN_VECTOR_FOREACH_END \
		} \
	} while (0)

dn_vector_t *
_dn_vector_alloc (
	dn_allocator_t *allocator,
	uint32_t element_size);

dn_vector_t *
_dn_vector_alloc_capacity (
	dn_allocator_t *allocator,
	uint32_t element_size,
	uint32_t capacity);

dn_vector_t *
_dn_vector_init (
	dn_vector_t *vector,
	dn_allocator_t *allocator,
	uint32_t version,
	uint32_t element_size);

dn_vector_t *
_dn_vector_init_capacity (
	dn_vector_t *vector,
	dn_allocator_t *allocator,
	uint32_t version,
	uint32_t element_size,
	uint32_t capacity);

bool
_dn_vector_insert_range (
	dn_vector_t *vector,
	uint32_t position,
	const uint8_t *elements,
	uint32_t element_count);

bool
_dn_vector_erase_if (
	dn_vector_t *vector,
	const uint8_t *value);

bool
_dn_vector_erase_fast_if (
	dn_vector_t *vector,
	const uint8_t *value);

uint32_t
_dn_vector_buffer_capacity (
	size_t buffer_size,
	uint32_t element_size);

#define dn_vector_custom_alloc_t(allocator, element_type) \
	_dn_vector_alloc ((allocator), sizeof (element_type));

#define dn_vector_alloc_t(element_type) \
	dn_vector_custom_alloc_t(DN_DEFAULT_ALLOCATOR, element_type)

#define dn_vector_custom_alloc_capacity_t(allocator, element_type, capacity) \
	_dn_vector_alloc_capacity ((allocator), sizeof (element_type), (capacity));

#define dn_vector_alloc_capacity_t(element_type, capacity) \
	dn_vector_custom_alloc_capacity_t (DN_DEFAULT_ALLOCATOR, element_type, capacity);

void
dn_vector_free (dn_vector_t *vector);

#define dn_vector_custom_init_t(vector, allocator, element_type) \
	_dn_vector_init ((vector), (allocator), sizeof (dn_vector_t), sizeof (element_type))

#define dn_vector_init_t(vector, element_type) \
	dn_vector_custom_init_t (vector, DN_DEFAULT_ALLOCATOR, element_type)

#define dn_vector_custom_init_capacity_t(vector, allocator, element_type, capacity) \
	_dn_vector_init_capacity ((vector), (allocator), sizeof (dn_vector_t), sizeof (element_type), (capacity))

#define dn_vector_init_capacity_t(vector, element_type, capacity) \
	dn_vector_custom_init_capacity_t (vector, DN_DEFAULT_ALLOCATOR, element_type, capacity)

void
dn_vector_fini (dn_vector_t *vector);

#define dn_vector_index_t(vector, type, index) \
	(type*)((uint8_t *)((vector)->data) + (sizeof (type) * (index)))

#define dn_vector_at_t(vector, type, index) \
	dn_vector_index_t(vector, type, index)

#define dn_vector_front_t(vector, type) \
	dn_vector_index_t(vector, type, 0)

#define dn_vector_back_t(vector, type) \
	dn_vector_index_t(vector, type, (vector)->size != 0 ? (vector)->size - 1 : 0)

#define dn_vector_data_t(vector,type) \
	((type *)(vector->data))

static inline uint32_t
dn_vector_begin (dn_vector_t *vector)
{
	DN_UNREFERENCED_PARAMETER (vector);
	return 0;
}

static inline uint32_t
dn_vector_end (dn_vector_t *vector)
{
	return vector->size;
}

static inline bool
dn_vector_empty (dn_vector_t *vector)
{
	if (!vector || vector->size == 0)
		return true;
	return false;
}

static inline uint32_t
dn_vector_size (dn_vector_t *vector)
{
	return vector->size;
}

static inline uint32_t
dn_vector_max_size (dn_vector_t *vector)
{
	DN_UNREFERENCED_PARAMETER (vector);
	return UINT32_MAX;
}

bool
dn_vector_reserve (
	dn_vector_t *vector,
	uint32_t capacity);

uint32_t
dn_vector_capacity (dn_vector_t *vector);

#define dn_vector_insert(vector, position, element) \
	_dn_vector_insert_range ((vector), (position), (const uint8_t *)&(element), 1)

#define dn_vector_insert_range(vector, position, elements, element_count) \
	_dn_vector_insert_range ((vector), (position), (const uint8_t *)(elements), (element_count))

bool
dn_vector_erase (
	dn_vector_t *vector,
	uint32_t position);

#define dn_vector_erase_if(vector, element) \
	_dn_vector_erase_if ((vector), (const uint8_t *)&(element))

bool
dn_vector_erase_fast (
	dn_vector_t *vector,
	uint32_t position);

#define dn_vector_erase_fast_if(vector, element) \
	_dn_vector_erase_fast_if ((vector), (const uint8_t *)&(element))

bool
dn_vector_resize (
	dn_vector_t *vector,
	uint32_t size);

static inline void
dn_vector_clear (dn_vector_t *vector)
{
	dn_vector_resize (vector, 0);
}

#define dn_vector_push_back(vector, element) \
	dn_vector_insert (vector, (vector)->size, element)

static inline void
dn_vector_pop_back (dn_vector_t *vector)
{
	dn_vector_erase_fast (vector, vector->size != 0 ? vector->size - 1 : 0);
}

#define dn_vector_buffer_capacity_t(buffer_byte_size, type) \
	_dn_vector_buffer_capacity ((buffer_byte_size), sizeof (type))

void
dn_vector_for_each (
	dn_vector_t *vector,
	dn_func_t func,
	void *user_data);

void
dn_vector_for_each_fini (
	dn_vector_t *vector,
	dn_fini_func_t func);

void
dn_vector_sort (
	dn_vector_t *vector,
	dn_compare_func_t func);

uint32_t
dn_vector_find (
	dn_vector_t *vector,
	const uint8_t *value);

#define DN_DEFINE_VECTOR_T_NAME(name) \
	dn_ ## name ## _vector_t

#define DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, symbol) \
	dn_ ## name ## _vector_ ## symbol

#define DN_DEFINE_VECTOR_T(name, type) \
DN_DEFINE_VECTOR_T_STRUCT(dn_ ## name ## _vector, type); \
typedef enum { \
	DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, vector_element_size) = sizeof (type), \
	DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, default_local_allocator_capacity_size) = 192, \
	DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, default_local_allocator_byte_size) = ((sizeof (type) * DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, default_local_allocator_capacity_size)) + DN_ALLOCATOR_ALIGN_SIZE (sizeof (dn_vector_t), DN_ALLOCATOR_MEM_ALIGN8) + 32) \
} DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, sizes); \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_alloc) (dn_allocator_t *allocator) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_t (allocator, type); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, alloc) (void) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_t (DN_DEFAULT_ALLOCATOR, type); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_alloc_capacity) (dn_allocator_t *allocator, uint32_t capacity) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_capacity_t (allocator, type, capacity); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, alloc_capacity) (uint32_t capacity) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_capacity_t (DN_DEFAULT_ALLOCATOR, type, capacity); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, free) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_free ((dn_vector_t *)vector); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_init) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_allocator_t *allocator) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_init_t ((dn_vector_t *)vector, allocator, type); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, init) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_init_t ((dn_vector_t *)vector, DN_DEFAULT_ALLOCATOR, type); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_init_capacity) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_allocator_t *allocator, uint32_t capacity) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_init_capacity_t ((dn_vector_t *)vector, allocator, type, capacity); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, init_capacity) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t capacity) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_init_capacity_t ((dn_vector_t *)vector, DN_DEFAULT_ALLOCATOR, type, capacity); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, fini) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_fini ((dn_vector_t *)vector); \
} \
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, index) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t index) \
{ \
	return (type*)(((vector)->data) + (index)); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, at) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t index) \
{ \
	return (type*)(((vector)->data) + (index)); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, front) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_front_t ((dn_vector_t *)vector, type); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, back) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_back_t ((dn_vector_t *)vector, type); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, data) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return ((type *)(vector->data)); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, begin) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_begin ((dn_vector_t *)vector); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, end) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_end ((dn_vector_t *)vector); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, empty) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_empty ((dn_vector_t *)vector); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, size) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_size ((dn_vector_t *)vector); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, max_size) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_max_size ((dn_vector_t *)vector); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, reserve) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t capacity) \
{ \
	return dn_vector_reserve ((dn_vector_t *)vector, capacity); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, capacity) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_capacity ((dn_vector_t *)vector); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, insert) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t position, type element) \
{ \
	return _dn_vector_insert_range ((dn_vector_t *)vector, position, (const uint8_t *)&(element), 1); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, insert_range) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t position, type *elements, uint32_t element_count) \
{ \
	return _dn_vector_insert_range ((dn_vector_t *)vector, position, (const uint8_t *)(elements), element_count); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, erase) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t position) \
{ \
	return dn_vector_erase ((dn_vector_t *)vector, position); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, erase_if) (DN_DEFINE_VECTOR_T_NAME(name) *vector, type element) \
{ \
	return _dn_vector_erase_if ((dn_vector_t *)vector, (const uint8_t *)&(element)); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, erase_fast) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t position) \
{ \
	return dn_vector_erase_fast ((dn_vector_t *)vector, position); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, erase_fast_if) (DN_DEFINE_VECTOR_T_NAME(name) *vector, type element) \
{ \
	return _dn_vector_erase_fast_if ((dn_vector_t *)vector, (const uint8_t *)&(element)); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, resize) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t size) \
{ \
	return dn_vector_resize ((dn_vector_t *)vector, size); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, clear) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_clear ((dn_vector_t *)vector); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, push_back) (DN_DEFINE_VECTOR_T_NAME(name) *vector, type element) \
{ \
	return dn_vector_push_back ((dn_vector_t *)vector, element); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, pop_back) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_pop_back ((dn_vector_t*)vector); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, buffer_capacity) (size_t buffer_byte_size) \
{ \
	return dn_vector_buffer_capacity_t (buffer_byte_size, type); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, for_each) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_func_t func, void *user_data) \
{ \
	dn_vector_for_each ((dn_vector_t*)vector, func, user_data); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, for_each_fini) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_fini_func_t func) \
{ \
	dn_vector_for_each_fini ((dn_vector_t*)vector, func); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, sort) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_compare_func_t func) \
{ \
	dn_vector_sort ((dn_vector_t*)vector, func); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, find) (DN_DEFINE_VECTOR_T_NAME(name) *vector, const type value) \
{ \
	return dn_vector_find ((dn_vector_t*)vector, (const uint8_t *)&(value)); \
}

#endif /* __DN_VECTOR_H__ */
