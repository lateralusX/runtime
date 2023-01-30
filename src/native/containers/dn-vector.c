// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "dn-vector.h"

#define CHECK_VERSION_RETURN_VALUE(vector,return_value) do { \
	if (DN_UNLIKELY ((vector)->version != sizeof (dn_vector_t))) \
		return (return_value); \
} while (0)

#define CHECK_VERSION_RETURN_VOID(vector) do { \
	if (DN_UNLIKELY ((vector)->version != sizeof (dn_vector_t))) \
		return; \
} while (0)

#define INITIAL_CAPACITY 16
#define CALC_NEW_CAPACITY(capacity) ((capacity + (capacity >> 1) + 63) & ~63)

#define element_offset(p,i) \
	((p)->data + (i) * (p)->_internal._element_size)

#define element_length(p,i) \
	((i) * (p)->_internal._element_size)

static bool
ensure_capacity (
	dn_vector_t *vector,
	uint32_t capacity)
{
	uint64_t new_capacity;

	if (capacity != 0 && capacity <= (uint64_t)(vector->_internal._capacity))
		return true;

	new_capacity = CALC_NEW_CAPACITY (capacity);

	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return false;

	size_t realloc_size;
	if (DN_UNLIKELY (!dn_safe_size_t_multiply (element_length (vector, 1), new_capacity, &realloc_size)))
		return false;

	uint8_t *data = (uint8_t *)dn_allocator_realloc (vector->_internal._allocator, vector->data, realloc_size);

	if (DN_UNLIKELY (!data && realloc_size != 0))
		return false;

	vector->data = data;

	if (vector->data && dn_allocator_init (vector->_internal._allocator)) {
		// Checks already verified that element_offset won't overflow.
		// new_capacity > vector capacity, so new_capacity - vector capacity won't underflow.
		// dn_safe_size_t_multiply already verified element_length won't overflow.
		memset (element_offset (vector, vector->_internal._capacity), 0, element_length (vector, (uint32_t)new_capacity - vector->_internal._capacity));
	}

	// Overflow already checked.
	vector->_internal._capacity = (uint32_t)new_capacity;

	return vector->data != NULL;
}

static bool
vector_insert_range (
	dn_vector_t *vector,
	uint32_t position,
	const uint8_t *elements,
	uint32_t element_count)
{
	uint64_t new_capacity = (uint64_t)vector->size + (uint64_t)element_count;
	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return false;

	if (DN_UNLIKELY (!ensure_capacity (vector, (uint32_t)new_capacity)))
		return false;

	uint64_t insert_offset = (uint64_t)position + (uint64_t)element_count;
	uint64_t size_to_move = (uint64_t)vector->size - (uint64_t)position;
	if (DN_UNLIKELY (insert_offset > new_capacity || size_to_move > vector->size))
		return false;

	/* first move the existing elements out of the way */
	/* element_offset won't overflow since insert_offset and position is smaller than new_capacity already checked in ensure_capacity */
	/* element_length won't overflow since size_to_move is smaller than new_capacity already checked in ensure_capacity */
	/* element_length won't underflow since size_to_move is already verfied to be smaller or equal to vector->size */
	memmove (element_offset (vector, insert_offset), element_offset (vector, (uint32_t)position), element_length (vector, size_to_move));

	/* then copy the new elements into the array */
	/* element_offset won't overflow since position is smaller than new_capacity already checked in encure_capacity */
	/* element_length won't overflow since element_count is included in new_capacity already checked in encure_capacity */
	memmove (element_offset (vector, (uint32_t)position), elements, element_length (vector, element_count));

	// Overflow already checked.
	vector->size += element_count;

	return true;
}

static bool
vector_append_range (
	dn_vector_t *vector,
	const uint8_t *elements,
	uint32_t element_count)
{
	uint64_t new_capacity = (uint64_t)vector->size + (uint64_t)element_count;
	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return false;
	
	if (DN_UNLIKELY (!ensure_capacity (vector, (uint32_t)new_capacity)))
		return false;

	/* ensure_capacity already verified element_offset and element_length won't overflow. */
	memmove (element_offset (vector, vector->size), elements, element_length (vector, element_count));

	// Overflowed already checked.
	vector->size += element_count;

	return true;
}

dn_vector_t *
_dn_vector_alloc (
	dn_allocator_t *allocator,
	uint32_t element_size)
{
	return _dn_vector_alloc_capacity (allocator, element_size, INITIAL_CAPACITY);
}

dn_vector_t *
_dn_vector_alloc_capacity (
	dn_allocator_t *allocator,
	uint32_t element_size,
	uint32_t capacity)
{
	dn_vector_t *vector = (dn_vector_t *)dn_allocator_alloc (allocator, sizeof (dn_vector_t));
	dn_vector_t *vector_init = _dn_vector_init_capacity (vector, allocator, sizeof (dn_vector_t), element_size, capacity);
	if (!vector_init)
		dn_allocator_free (allocator, vector);

	return vector_init;
}

dn_vector_t *
_dn_vector_init (
	dn_vector_t *vector,
	dn_allocator_t *allocator,
	uint32_t version,
	uint32_t element_size)
{
	return _dn_vector_init_capacity (vector, allocator, version, element_size, INITIAL_CAPACITY);
}

dn_vector_t *
_dn_vector_init_capacity (
	dn_vector_t *vector,
	dn_allocator_t *allocator,
	uint32_t version,
	uint32_t element_size,
	uint32_t capacity)
{
	if (DN_UNLIKELY (!vector || version != sizeof (dn_vector_t)))
		return NULL;

	memset (vector, 0, sizeof(dn_vector_t));
	vector->version = version;
	vector->_internal._allocator = allocator;
	vector->_internal._element_size = element_size;

	if (DN_UNLIKELY (!ensure_capacity (vector, capacity))) {
		dn_vector_fini (vector);
		return NULL;
	}

	return vector;
}

bool
_dn_vector_insert_range (
	dn_vector_t *vector,
	uint32_t position,
	const uint8_t *elements,
	uint32_t element_count)
{
	CHECK_VERSION_RETURN_VALUE (vector, false);

	if ((uint32_t)position == vector->size)
		return vector_append_range (vector, elements, element_count);
	else
		return vector_insert_range (vector, position, elements, element_count);
}

bool
_dn_vector_erase_if (
	dn_vector_t *vector,
	const uint8_t *value)
{
	CHECK_VERSION_RETURN_VALUE (vector, false);

	for (uint32_t i = 0; i < vector->size; i++) {
		if (!memcmp (element_offset (vector, i), value, element_length (vector, 1))) {
			if (dn_vector_erase (vector, i))
				return true;
		}
	}

	return false;
}

bool
_dn_vector_erase_fast_if (
	dn_vector_t *vector,
	const uint8_t *value)
{
	CHECK_VERSION_RETURN_VALUE (vector, false);

	for (uint32_t i = 0; i < vector->size; i++) {
		if (!memcmp (element_offset (vector, i), value, element_length (vector, 1))) {
			if (dn_vector_erase_fast (vector, i))
				return true;
		}
	}

	return false;
}

uint32_t
_dn_vector_buffer_capacity (
	size_t buffer_byte_size,
	uint32_t element_size)
{
	// Estimate maximum array capacity for buffer size.
	size_t max_capacity = (buffer_byte_size - DN_ALLOCATOR_ALIGN_SIZE (sizeof (dn_vector_t), DN_ALLOCATOR_MEM_ALIGN8) - 32 /* padding */) / element_size;
	if (DN_UNLIKELY (max_capacity > buffer_byte_size || max_capacity > (size_t)UINT32_MAX))
		return 0;

	// Adjust to heuristics in ensure_capacity.
	uint32_t capacity = 1;
	while(CALC_NEW_CAPACITY (capacity) <= (uint32_t)max_capacity)
		capacity <<= 1;

	return (uint32_t)(capacity >> 1);
}

void
dn_vector_free (dn_vector_t *vector)
{
	dn_vector_fini (vector);
	dn_allocator_free (vector->_internal._allocator, vector);
}

void
dn_vector_fini (dn_vector_t *vector)
{
	if (vector)
		dn_allocator_free (vector->_internal._allocator, vector->data);
}

bool
dn_vector_reserve (
	dn_vector_t *vector,
	uint32_t capacity)
{
	return ensure_capacity (vector, capacity);
}

uint32_t
dn_vector_capacity (dn_vector_t *vector)
{
	return vector->_internal._capacity;
}

bool
dn_vector_erase (
	dn_vector_t *vector,
	uint32_t position)
{
	CHECK_VERSION_RETURN_VALUE (vector, false);

	uint64_t insert_offset = (uint64_t)position + 1;
	int64_t size_to_move = (int64_t)vector->size - (int64_t)position;
	if (DN_UNLIKELY (insert_offset > vector->_internal._capacity || size_to_move < 0))
		return false;
	
	/* element_offset won't overflow since insert_offset and position is smaller than current capacity */
	/* element_length won't overflow since size_to_move is smaller than current capacity */
	/* element_length won't underflow since size_to_move is already verfied to be 0 or larger */
	memmove (element_offset (vector, (uint32_t)position), element_offset (vector, insert_offset), element_length (vector, size_to_move));

	if (dn_allocator_init (vector->_internal._allocator))
		memset (element_offset(vector, vector->size - 1), 0, element_length (vector, 1));

	vector->size --;
	return true;
}

bool
dn_vector_erase_fast (
	dn_vector_t *vector,
	uint32_t position)
{
	CHECK_VERSION_RETURN_VALUE (vector, false);

	if (DN_UNLIKELY (vector->size == 0 || (uint32_t)position > vector->size))
		return false;

	/* element_offset won't overflow since position is smaller than current capacity */
	/* element_offset won't overflow since vector->size - 1 is smaller than current capacity */
	/* vector->size - 1 won't underflow since vector->size > 0 */
	memmove (element_offset (vector, (uint32_t)position), element_offset (vector, vector->size - 1), element_length (vector, 1));
	
	if (dn_allocator_init (vector->_internal._allocator))
		memset (element_offset(vector, vector->size - 1), 0, element_length (vector, 1));

	vector->size --;
	return true;
}

bool
dn_vector_resize (
	dn_vector_t *vector,
	uint32_t size)
{
	CHECK_VERSION_RETURN_VALUE (vector, false);

	if (size == vector->_internal._capacity)
		return true;

	if (size > vector->_internal._capacity)
		if (DN_UNLIKELY (!ensure_capacity (vector, size)))
			return false;
	
	//TODO: If new size < vector->size check dn_allocator_init and clear if needed.
	vector->size = size;
	return true;
}

void
dn_vector_for_each (
	dn_vector_t *vector,
	dn_func_t func,
	void *user_data)
{
	CHECK_VERSION_RETURN_VOID (vector);

	if (vector && func) {
		for(uint32_t i = 0; i < vector->size; i++) {
			func ((void *)(element_offset (vector, i)), user_data);
		}
	}
}

void
dn_vector_for_each_fini (
	dn_vector_t *vector,
	dn_fini_func_t func)
{
	CHECK_VERSION_RETURN_VOID (vector);

	if (vector && func) {
		for (uint32_t i = 0; i < vector->size; ++i)
			func ((void *)(element_offset (vector, i)));
	}
}

void
dn_vector_sort (
	dn_vector_t *vector,
	dn_compare_func_t func)
{
	if (vector == NULL || vector->size < 2)
		return;
	qsort ((void *)vector->data, vector->size, element_length (vector, 1), (int (DN_CALLBACK_CALLTYPE *)(const void *, const void *))func);
}

uint32_t
dn_vector_find (
	dn_vector_t *vector,
	const uint8_t *value)
{
	if (vector == NULL)
		return 0;

	for (uint32_t i = 0; i < vector->size; i++) {
		if (!memcmp (element_offset (vector, i), value, element_length (vector, 1))) {
			return i;
		}
	}

	return dn_vector_end (vector);
}
