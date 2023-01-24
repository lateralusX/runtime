#include "dn-vector.h"
#include "dn-malloc.h"

#define INITIAL_CAPACITY 16

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

	new_capacity = (capacity + (capacity >> 1) + 63) & ~63;

	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return false;

	size_t realloc_size;
	if (DN_UNLIKELY (!dn_safe_size_t_multiply (vector->_internal._element_size, new_capacity, &realloc_size)))
		return false;

	uint8_t *data = (uint8_t *)dn_realloc (vector->data, realloc_size);
	if (DN_UNLIKELY (!data && realloc_size != 0))
		return false;

	vector->data = data;

	if (vector->_internal._clear && vector->data) {
		// Checks already verified that element_offset won't overflow.
		// new_capacity > vector capacity, so new_capacity - vector capacity won't underflow.
		// dn_safe_size_t_multiply already verified element_length won't overflow.
		memset (element_offset (vector, vector->_internal._capacity), 0, element_length (vector, (uint32_t)new_capacity - vector->_internal._capacity));
	}

	// Overflow already checked.
	vector->_internal._capacity = (uint32_t)new_capacity;

	return vector->data != NULL;
}

dn_vector_t *
dn_vector_alloc (uint32_t element_size)
{
	return dn_vector_alloc_capacity (element_size, INITIAL_CAPACITY);
}

dn_vector_t *
dn_vector_alloc_capacity (
	uint32_t element_size,
	uint32_t capacity)
{
	dn_vector_t *vector = dn_malloc (sizeof (dn_vector_t));
	dn_vector_t *vector_init = dn_vector_init_capacity (vector, sizeof (dn_vector_t), element_size, capacity);
	if (!vector_init)
		dn_free (vector);
	return vector_init;
}

void
dn_vector_free (dn_vector_t *vector)
{
	dn_vector_fini (vector);
	dn_free (vector);
}

dn_vector_t *
dn_vector_init (
	dn_vector_t *vector,
	uint32_t version,
	uint32_t element_size)
{
	return dn_vector_init_capacity (vector, version, element_size, INITIAL_CAPACITY);
}

dn_vector_t *
dn_vector_init_capacity (
	dn_vector_t *vector,
	uint32_t version,
	uint32_t element_size,
	uint32_t capacity)
{
	if (DN_UNLIKELY (!vector || version != sizeof (dn_vector_t)))
		return NULL;

	memset (vector, 0, sizeof(dn_vector_t));
	vector->version = version;
	vector->_internal._element_size = element_size;
	vector->_internal._clear = true;

	if (DN_UNLIKELY (!ensure_capacity (vector, capacity))) {
		dn_vector_fini (vector);
		return NULL;
	}

	return vector;
}

void
dn_vector_fini (dn_vector_t *vector)
{
	if (vector)
		dn_free (vector->data);
}

bool
dn_vector_reserve (
	dn_vector_t *vector,
	uint32_t capacity)
{
	return ensure_capacity (vector, capacity);
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

bool
dn_vector_insert_range (
	dn_vector_t *vector,
	uint32_t position,
	const uint8_t *elements,
	uint32_t element_count)
{
	if (DN_UNLIKELY (vector->version != sizeof (dn_vector_t)))
		return false;

	if ((uint32_t)position == vector->size)
		return vector_append_range (vector, elements, element_count);
	else
		return vector_insert_range (vector, position, elements, element_count);
}

bool
dn_vector_erase (
	dn_vector_t *vector,
	uint32_t position)
{
	if (DN_UNLIKELY (vector->version != sizeof (dn_vector_t)))
		return false;

	uint64_t insert_offset = (uint64_t)position + 1;
	int64_t size_to_move = (int64_t)vector->size - (int64_t)position;
	if (DN_UNLIKELY (insert_offset > vector->_internal._capacity || size_to_move < 0))
		return false;
	
	/* element_offset won't overflow since insert_offset and position is smaller than current capacity */
	/* element_length won't overflow since size_to_move is smaller than current capacity */
	/* element_length won't underflow since size_to_move is already verfied to be 0 or larger */
	memmove (element_offset (vector, (uint32_t)position), element_offset (vector, insert_offset), element_length (vector, size_to_move));

	vector->size --;
	return true;
}

bool
dn_vector_erase_fast (
	dn_vector_t *vector,
	uint32_t position)
{
	if (DN_UNLIKELY (vector->version != sizeof (dn_vector_t)))
		return false;

	if (DN_UNLIKELY (vector->size == 0 || (uint32_t)position > vector->size))
		return false;

	/* element_offset won't overflow since position is smaller than current capacity */
	/* element_offset won't overflow since vector->size - 1 is smaller than current capacity */
	/* vector->size - 1 won't underflow since vector->size > 0 */
	memmove (element_offset (vector, (uint32_t)position), element_offset (vector, vector->size - 1), element_length (vector, 1));

	vector->size --;
	return true;
}

bool
dn_vector_resize (
	dn_vector_t *vector,
	uint32_t size)
{
	if (DN_UNLIKELY (vector->version != sizeof (dn_vector_t)))
		return false;

	if (size == vector->_internal._capacity)
		return true;

	if (size > vector->_internal._capacity)
		if (DN_UNLIKELY (!ensure_capacity (vector, size)))
			return false;

	vector->size = size;
	return true;
}
