
#ifndef __DN_VECTOR_H__
#define __DN_VECTOR_H__

#include "dn-utils.h"

typedef struct _dn_vector_t dn_vector_t;
struct _dn_vector_t {
	uint8_t *data;
	uint32_t size;
	uint32_t version;
	struct {
		uint32_t _element_size;
		uint32_t _capacity;
		bool _clear;
	}_internal;
};

#define DN_VECTOR_FOREACH_BEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	for (uint32_t __i_##var_name = 0; __i_##var_name < (vector)->size; ++__i_##var_name) { \
		var_name = dn_vector_index_t (vector, var_type, __i_##var_name);

#define DN_VECTOR_FOREACH_RBEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	for (uint32_t __i_##var_name = (vector)->size; __i_##var_name > 0; --__i_##var_name) { \
		var_name = dn_vector_index_t (vector, var_type, __i_##var_name - 1);

#define DN_VECTOR_FOREACH_END \
		} \
	} while (0)

dn_vector_t *
dn_vector_alloc (uint32_t element_size);

#define dn_vector_alloc_t(element_type) \
	dn_vector_alloc (sizeof (element_type));

dn_vector_t *
dn_vector_alloc_capacity (
	uint32_t element_size,
	uint32_t capacity);

#define dn_vector_alloc_capacity_t(element_type, capacity) \
	dn_vector_alloc_capacity (sizeof (element_type), (capacity));

void
dn_vector_free (dn_vector_t *vector);

dn_vector_t *
dn_vector_init (
	dn_vector_t *vector,
	uint32_t version,
	uint32_t element_size);

#define dn_vector_init_t(vector, element_type) \
	dn_vector_init ((vector), sizeof (dn_vector_t), sizeof (element_type))

dn_vector_t *
dn_vector_init_capacity (
	dn_vector_t *vector,
	uint32_t version,
	uint32_t element_size,
	uint32_t capacity);

#define dn_vector_init_capacity_t(vector, element_type, capacity) \
	dn_vector_init_capacity ((vector), sizeof (dn_vector_t), sizeof (element_type), (capacity))

void
dn_vector_fini (dn_vector_t *vector);

#define dn_vector_index(vector, index) \
	(((vector)->data) + (vector)->_internal._element_size * (index))

#define dn_vector_index_t(vector, type, index) \
	*(type*)(((vector)->data) + sizeof(type) * (index))

#define dn_vector_front(vector) \
	dn_vector_index(vector, 0)

#define dn_vector_front_t(vector, type) \
	dn_vector_index_t(vector, type, 0)

#define dn_vector_back(vector) \
	dn_vector_index(vector, (vector)->size != 0 ? (vector)->size - 1 : 0)

#define dn_vector_back_t(vector, type) \
	dn_vector_index_t(vector, type, (vector)->size != 0 ? (vector)->size - 1 : 0)

static inline uint8_t *
dn_vector_data (dn_vector_t *vector)
{
	return vector->data;
}

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

static inline uint32_t
dn_vector_capacity (dn_vector_t *vector)
{
	return vector->_internal._capacity;
}

bool
dn_vector_insert_range (
	dn_vector_t *vector,
	uint32_t position,
	const uint8_t *elements,
	uint32_t element_count);

#define dn_vector_insert(vector, position, element) \
	dn_vector_insert_range ((vector), (position), (const uint8_t *)&(element), 1)

bool
dn_vector_erase (
	dn_vector_t *vector,
	uint32_t position);

bool
dn_vector_erase_fast (
	dn_vector_t *vector,
	uint32_t position);

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
	dn_vector_insert ((vector), (vector)->size, (element))

static inline void
dn_vector_pop_back (dn_vector_t *vector)
{
	dn_vector_erase_fast (vector, vector->size != 0 ? vector->size - 1 : 0);
}

#endif /* __DN_VECTOR_H__ */
