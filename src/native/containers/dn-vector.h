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
	struct { \
		uint32_t _element_size; \
		uint32_t _capacity; \
		uint32_t _attributes; \
		dn_allocator_t *_allocator; \
	}_internal; \
}; \
typedef struct _ ## name ## _it_t name ## _it_t; \
struct _ ## name ## _it_t { \
	uint32_t it; \
	struct { \
		name ## _t *_vector; \
	} _internal; \
};

DN_DEFINE_VECTOR_T_STRUCT(dn_vector, uint8_t);

typedef enum {
	DN_VECTOR_ATTRIBUTE_INIT_MEMORY = 0x1
} dn_vector_attribute;

static inline void
dn_vector_it_advance (
	dn_vector_it_t *it,
	ptrdiff_t n)
{
	it->it = it->it + n;
}

static inline dn_vector_it_t
dn_vector_it_next (dn_vector_it_t it)
{
	it.it++;
	return it;
}

static inline dn_vector_it_t
dn_vector_it_prev (dn_vector_it_t it)
{
	it.it--;
	return it;
}

static inline dn_vector_it_t
dn_vector_it_next_n (
	dn_vector_it_t it,
	uint32_t n)
{
	it.it += n;
	return it;
}

static inline dn_vector_it_t
dn_vector_it_prev_n (
	dn_vector_it_t it,
	uint32_t n)
{
	it.it -= n;
	return it;
}

static inline uint8_t *
dn_vector_it_data (dn_vector_it_t it)
{
	return it._internal._vector->data + (it._internal._vector->_internal._element_size * it.it);
}

#define dn_vector_it_data_t(it, type) \
	(type *)dn_vector_it_data ((it))

static inline bool
dn_vector_it_begin (dn_vector_it_t it)
{
	return it.it == 0;
}

static inline bool
dn_vector_it_end (dn_vector_it_t it)
{
	return it.it == it._internal._vector->size;
}

#define DN_VECTOR_FOREACH_BEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	DN_ASSERT (sizeof (var_type) == (vector)->_internal._element_size); \
	for (uint32_t __i_ ## var_name = 0; __i_ ## var_name < (vector)->size; ++__i_ ## var_name) { \
		var_name = *dn_vector_index_t (vector, var_type, __i_##var_name);

#define DN_VECTOR_FOREACH_RBEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	DN_ASSERT (sizeof (var_type) == (vector)->_internal._element_size); \
	for (uint32_t __i_ ## var_name = (vector)->size; __i_ ## var_name > 0; --__i_ ## var_name) { \
		var_name = *dn_vector_index_t (vector, var_type, __i_ ## var_name - 1);

#define DN_VECTOR_FOREACH_END \
		} \
	} while (0)

dn_vector_t *
_dn_vector_alloc (
	dn_allocator_t *allocator,
	uint32_t element_size,
	uint32_t attributes);

dn_vector_t *
_dn_vector_alloc_capacity (
	dn_allocator_t *allocator,
	uint32_t element_size,
	uint32_t capacity,
	uint32_t attributes);

bool
_dn_vector_init (
	dn_vector_t *vector,
	dn_allocator_t *allocator,
	uint32_t element_size,
	uint32_t attributes);

bool
_dn_vector_init_capacity (
	dn_vector_t *vector,
	dn_allocator_t *allocator,
	uint32_t element_size,
	uint32_t capacity,
	uint32_t attributes);

bool
_dn_vector_insert_range (
	dn_vector_it_t *position,
	const uint8_t *elements,
	uint32_t element_count);

bool
_dn_vector_append_range (
	dn_vector_t *vector,
	const uint8_t *elements,
	uint32_t element_count);

bool
_dn_vector_erase (
	dn_vector_it_t *position,
	dn_func_t func);

bool
_dn_vector_erase_fast (
	dn_vector_it_t *position,
	dn_func_t func);

uint32_t
_dn_vector_buffer_capacity (
	size_t buffer_size,
	uint32_t element_size);

#define dn_vector_custom_alloc_t(allocator, element_type, attributes) \
	_dn_vector_alloc ((allocator), sizeof (element_type), (attributes));

#define dn_vector_alloc_t(element_type) \
	dn_vector_custom_alloc_t (DN_DEFAULT_ALLOCATOR, element_type, DN_VECTOR_ATTRIBUTE_INIT_MEMORY)

#define dn_vector_custom_alloc_capacity_t(allocator, element_type, capacity, attributes) \
	_dn_vector_alloc_capacity ((allocator), sizeof (element_type), (capacity), (attributes));

#define dn_vector_alloc_capacity_t(element_type, capacity) \
	dn_vector_custom_alloc_capacity_t (DN_DEFAULT_ALLOCATOR, element_type, capacity, DN_VECTOR_ATTRIBUTE_INIT_MEMORY);

void
dn_vector_custom_free (
	dn_vector_t *vector,
	dn_func_t func);

static inline void
dn_vector_free (dn_vector_t *vector)
{
	dn_vector_custom_free (vector, NULL);
}

#define dn_vector_custom_init_t(vector, allocator, element_type, attributes) \
	_dn_vector_init ((vector), (allocator), sizeof (element_type), (attributes))

#define dn_vector_init_t(vector, element_type) \
	dn_vector_custom_init_t (vector, DN_DEFAULT_ALLOCATOR, element_type, DN_VECTOR_ATTRIBUTE_INIT_MEMORY)

#define dn_vector_custom_init_capacity_t(vector, allocator, element_type, capacity, attributes) \
	_dn_vector_init_capacity ((vector), (allocator), sizeof (element_type), (capacity), (attributes))

#define dn_vector_init_capacity_t(vector, element_type, capacity) \
	dn_vector_custom_init_capacity_t (vector, DN_DEFAULT_ALLOCATOR, element_type, capacity, DN_VECTOR_ATTRIBUTE_INIT_MEMORY)

void
dn_vector_custom_dispose (
	dn_vector_t *vector,
	dn_func_t func);

static inline void
dn_vector_dispose (dn_vector_t *vector)
{
	dn_vector_custom_dispose (vector, NULL);
}

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

static inline dn_vector_it_t
dn_vector_begin (dn_vector_t *vector)
{
	dn_vector_it_t it = { 0, { vector } };
	return it;
}

static inline dn_vector_it_t
dn_vector_end (dn_vector_t *vector)
{
	dn_vector_it_t it = { vector->size, { vector } };
	return it;
}

static inline bool
dn_vector_empty (const dn_vector_t *vector)
{
	if (!vector || vector->size == 0)
		return true;
	return false;
}

static inline uint32_t
dn_vector_size (const dn_vector_t *vector)
{
	return vector->size;
}

static inline uint32_t
dn_vector_max_size (const dn_vector_t *vector)
{
	DN_UNREFERENCED_PARAMETER (vector);
	return UINT32_MAX;
}

bool
dn_vector_reserve (
	dn_vector_t *vector,
	uint32_t capacity);

uint32_t
dn_vector_capacity (const dn_vector_t *vector);

static inline dn_vector_it_t
_dn_vector_insert_range_adapter (
	dn_vector_it_t position,
	const uint8_t *elements,
	uint32_t element_count,
	bool *result)
{
	bool insert_result;
	if (dn_vector_it_end (position))
		insert_result = _dn_vector_append_range (position._internal._vector, elements, element_count);
	else
		insert_result = _dn_vector_insert_range (&position, elements, element_count);

	if (result)
		*result = insert_result;
	return position;
}

#define dn_vector_insert(position, element, result) \
	_dn_vector_insert_range_adapter ((position), (const uint8_t *)&(element), 1, (result))

#define dn_vector_insert_range(position, elements, element_count, result) \
	_dn_vector_insert_range_adapter ((position), (const uint8_t *)(elements), (element_count), (result))

static inline dn_vector_it_t
_dn_vector_erase_adapter (
	dn_vector_it_t position,
	dn_func_t func,
	bool *result)
{
	bool erase_result = _dn_vector_erase (&position, func);
	if (result)
		*result = erase_result;
	return position;
}

#define dn_vector_custom_erase(position, func, result) \
	_dn_vector_erase_adapter ((position), (func), (result))

#define dn_vector_erase(position, result) \
	_dn_vector_erase_adapter ((position), NULL, (result))

static inline dn_vector_it_t
_dn_vector_erase_fast_adapter (
	dn_vector_it_t position,
	dn_func_t func,
	bool *result)
{
	bool erase_result = _dn_vector_erase_fast (&position, func);
	if (result)
		*result = erase_result;
	return position;
}

#define dn_vector_custom_erase_fast(position, func, result) \
	_dn_vector_erase_fast_adapter ((position), (func), (result))

#define dn_vector_erase_fast(position, result) \
	_dn_vector_erase_fast_adapter ((position), NULL, (result))

bool
dn_vector_custom_resize (
	dn_vector_t *vector,
	uint32_t size,
	dn_func_t func);

static inline bool
dn_vector_resize (
	dn_vector_t *vector,
	uint32_t size)
{
	return dn_vector_custom_resize (vector, size, NULL);
}

static inline void
dn_vector_custom_clear (
	dn_vector_t *vector,
	dn_func_t func)
{
	dn_vector_custom_resize (vector, 0, func);
}

static inline void
dn_vector_clear (dn_vector_t *vector)
{
	dn_vector_resize (vector, 0);
}

#define dn_vector_push_back(vector, element) \
	_dn_vector_append_range ((vector), (const uint8_t *)&(element), 1)

static inline void
dn_vector_pop_back (dn_vector_t *vector)
{
	if (DN_UNLIKELY (!vector || vector->size == 0))
		return;
	//TODO: Check clear.
	vector->size --;
}

#define dn_vector_buffer_capacity_t(buffer_byte_size, type) \
	_dn_vector_buffer_capacity ((buffer_byte_size), sizeof (type))

void
dn_vector_for_each (
	const dn_vector_t *vector,
	dn_func_data_t foreach_func,
	void *data);

void
dn_vector_sort (
	dn_vector_t *vector,
	dn_compare_func_t compare_func);

dn_vector_it_t
dn_vector_find (
	dn_vector_t *vector,
	const uint8_t *value,
	dn_compare_func_t compare_func);

static inline void
_dn_vector_find_adapter (
	dn_vector_t *vector,
	const uint8_t *data,
	dn_compare_func_t compare_func,
	dn_vector_it_t *found)
{
	*found = dn_vector_find (vector, data, compare_func);
}

#define DN_DEFINE_VECTOR_T_NAME(name) \
	dn_ ## name ## _vector_t

#define DN_DEFINE_VECTOR_IT_T_NAME(name) \
	dn_ ## name ## _vector_it_t

#define DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, symbol) \
	dn_ ## name ## _vector_ ## symbol

#define DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, symbol) \
	dn_ ## name ## _vector_it_ ## symbol

#define DN_DEFINE_VECTOR_T(name, type) \
DN_DEFINE_VECTOR_T_STRUCT(dn_ ## name ## _vector, type); \
typedef enum { \
	DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, vector_element_size) = sizeof (type), \
	DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, default_local_allocator_capacity_size) = 192, \
	DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, default_local_allocator_byte_size) = ((sizeof (type) * DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, default_local_allocator_capacity_size)) + DN_ALLOCATOR_ALIGN_SIZE (sizeof (dn_vector_t), DN_ALLOCATOR_MEM_ALIGN8) + 32) \
} DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, sizes); \
static inline void \
DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, advance) (DN_DEFINE_VECTOR_IT_T_NAME(name) *it, ptrdiff_t n) \
{ \
	dn_vector_it_advance ((dn_vector_it_t *)it, n); \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, next) (DN_DEFINE_VECTOR_IT_T_NAME(name) it) \
{ \
	it.it++; \
	return it; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, prev) (DN_DEFINE_VECTOR_IT_T_NAME(name) it) \
{ \
	it.it--; \
	return it; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, next_n) (DN_DEFINE_VECTOR_IT_T_NAME(name) it, uint32_t n) \
{ \
	it.it -= n; \
	return it; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, prev_n) (DN_DEFINE_VECTOR_IT_T_NAME(name) it, uint32_t n) \
{ \
	it.it += n; \
	return it; \
} \
static inline type * \
DN_DEFINE_VECTOR_IT_T_SYMBOL_NAME(name, data) (DN_DEFINE_VECTOR_IT_T_NAME(name) it, uint32_t n) \
{ \
	return (type *)(it._internal._vector->data + n);\
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_alloc) (dn_allocator_t *allocator, uint32_t attributes) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_t (allocator, type, attributes); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, alloc) (void) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_t (DN_DEFAULT_ALLOCATOR, type, DN_VECTOR_ATTRIBUTE_INIT_MEMORY); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_alloc_capacity) (dn_allocator_t *allocator, uint32_t capacity, uint32_t attributes) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_capacity_t (allocator, type, capacity, attributes); \
} \
static inline DN_DEFINE_VECTOR_T_NAME(name) * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, alloc_capacity) (uint32_t capacity) \
{ \
	return (DN_DEFINE_VECTOR_T_NAME(name) *)dn_vector_custom_alloc_capacity_t (DN_DEFAULT_ALLOCATOR, type, capacity, DN_VECTOR_ATTRIBUTE_INIT_MEMORY); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, free) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_free ((dn_vector_t *)vector); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_free) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_func_t func) \
{ \
	dn_vector_custom_free ((dn_vector_t *)vector, func); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_init) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_allocator_t *allocator, uint32_t attributes) \
{ \
	return dn_vector_custom_init_t ((dn_vector_t *)vector, allocator, type, attributes); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, init) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_custom_init_t ((dn_vector_t *)vector, DN_DEFAULT_ALLOCATOR, type, DN_VECTOR_ATTRIBUTE_INIT_MEMORY); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_init_capacity) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_allocator_t *allocator, uint32_t capacity, uint32_t attributes) \
{ \
	return dn_vector_custom_init_capacity_t ((dn_vector_t *)vector, allocator, type, capacity, attributes); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, init_capacity) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t capacity) \
{ \
	return dn_vector_custom_init_capacity_t ((dn_vector_t *)vector, DN_DEFAULT_ALLOCATOR, type, capacity, DN_VECTOR_ATTRIBUTE_INIT_MEMORY); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_dispose) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_func_t func) \
{ \
	dn_vector_custom_dispose ((dn_vector_t *)vector, func); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, dispose) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_custom_dispose ((dn_vector_t *)vector, NULL); \
} \
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, index) (const DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t index) \
{ \
	return (type*)(((vector)->data) + (index)); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, at) (const DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t index) \
{ \
	return (type*)(((vector)->data) + (index)); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, front) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_front_t ((dn_vector_t *)vector, type); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, back) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_back_t ((dn_vector_t *)vector, type); \
}\
static inline type * \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, data) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return ((type *)(vector->data)); \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, begin) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	DN_DEFINE_VECTOR_IT_T_NAME(name) it = { 0, { vector } }; \
	return it; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, end) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	DN_DEFINE_VECTOR_IT_T_NAME(name) it = { vector->size, { vector } }; \
	return it; \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, empty) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_empty ((dn_vector_t *)vector); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, size) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_size ((dn_vector_t *)vector); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, max_size) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_max_size ((dn_vector_t *)vector); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, reserve) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t capacity) \
{ \
	return dn_vector_reserve ((dn_vector_t *)vector, capacity); \
} \
static inline uint32_t \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, capacity) (const DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	return dn_vector_capacity ((dn_vector_t *)vector); \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, insert) (DN_DEFINE_VECTOR_IT_T_NAME(name) position, type element, bool *result) \
{ \
	bool insert_result = _dn_vector_insert_range ((dn_vector_it_t *)&position, (const uint8_t *)&element, 1); \
	if (result) \
		*result = insert_result; \
	return position; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, insert_range) (DN_DEFINE_VECTOR_IT_T_NAME(name) position, type *elements, uint32_t element_count, bool *result) \
{ \
	bool insert_result = _dn_vector_insert_range ((dn_vector_it_t *)&position, (const uint8_t *)(elements), element_count); \
	if (result) \
		*result = insert_result; \
	return position; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_erase) (DN_DEFINE_VECTOR_IT_T_NAME(name) position, dn_func_t func, bool *result) \
{ \
	bool erase_result = _dn_vector_erase ((dn_vector_it_t *)&position, func); \
	if (result) \
		*result = erase_result; \
	return position; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, erase) (DN_DEFINE_VECTOR_IT_T_NAME(name) position, bool *result) \
{ \
	return DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_erase) (position, NULL, result); \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_erase_fast) (DN_DEFINE_VECTOR_IT_T_NAME(name) position, dn_func_t func, bool *result) \
{ \
	bool erase_result = _dn_vector_erase_fast ((dn_vector_it_t *)&position, func); \
	if (result) \
		*result = erase_result; \
	return position; \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, erase_fast) (DN_DEFINE_VECTOR_IT_T_NAME(name) position, bool *result) \
{ \
	return DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_erase_fast) (position, NULL, result); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_resize) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t size, dn_func_t func) \
{ \
	return dn_vector_custom_resize ((dn_vector_t *)vector, size, func); \
} \
static inline bool \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, resize) (DN_DEFINE_VECTOR_T_NAME(name) *vector, uint32_t size) \
{ \
	return dn_vector_custom_resize ((dn_vector_t *)vector, size, NULL); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, custom_clear) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_func_t func) \
{ \
	dn_vector_custom_clear ((dn_vector_t *)vector, func); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, clear) (DN_DEFINE_VECTOR_T_NAME(name) *vector) \
{ \
	dn_vector_custom_clear ((dn_vector_t *)vector, NULL); \
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
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, for_each) (const DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_func_data_t foreach_func, void *data) \
{ \
	dn_vector_for_each ((dn_vector_t*)vector, foreach_func, data); \
} \
static inline void \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, sort) (DN_DEFINE_VECTOR_T_NAME(name) *vector, dn_compare_func_t compare_func) \
{ \
	dn_vector_sort ((dn_vector_t*)vector, compare_func); \
} \
static inline DN_DEFINE_VECTOR_IT_T_NAME(name) \
DN_DEFINE_VECTOR_T_SYMBOL_NAME(name, find) (const DN_DEFINE_VECTOR_T_NAME(name) *vector, const type value, dn_compare_func_t compare_func) \
{ \
	DN_DEFINE_VECTOR_IT_T_NAME(name) found; \
	_dn_vector_find_adapter ((dn_vector_t*)vector, (const uint8_t *)&value, compare_func, (dn_vector_it_t *)&found); \
	return found; \
}

#endif /* __DN_VECTOR_H__ */
