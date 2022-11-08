#include "dn-allocator.h"
#include "dn-malloc.h"

static void *
static_vtable_alloc (
	dn_allocator_t *allocator,
	size_t size);

static void *
static_vtable_realloc (
	dn_allocator_t *allocator,
	void *block,
	size_t size);

static void
static_vtable_free (
	dn_allocator_t *allocator,
	void *block);

static void *
static_alloc (
	dn_allocator_static_data_t *data,
	size_t size);

static void *
static_realloc (
	dn_allocator_static_data_t *data,
	void *block,
	size_t size);

static void
static_free (
	dn_allocator_static_data_t *data,
	void *block);

static bool
static_owns_ptr (
	dn_allocator_static_data_t *data,
	void *ptr);

static void *
static_memcpy (
	void *dst,
	void *src,
	size_t size);

static void *
static_dynamic_vtable_alloc (
	dn_allocator_t *allocator,
	size_t size);

static void *
static_dynamic_vtable_realloc (
	dn_allocator_t *allocator,
	void *block,
	size_t size);

static void
static_dynamic_vtable_free (
	dn_allocator_t *allocator,
	void *block);

static void *
static_dynamic_alloc (
	dn_allocator_static_dynamic_data_t *data,
	size_t size);

static void *
static_dynamic_realloc (
	dn_allocator_static_dynamic_data_t *data,
	void *block,
	size_t size);

static void
static_dynamic_free (
	dn_allocator_static_dynamic_data_t *data,
	void *block);

static dn_allocator_vtable_t static_vtable = {
	static_vtable_alloc,
	static_vtable_realloc,
	static_vtable_free
};

static dn_allocator_vtable_t static_dynamic_vtable = {
	static_dynamic_vtable_alloc,
	static_dynamic_vtable_realloc,
	static_dynamic_vtable_free
};

static void *
static_vtable_alloc (
	dn_allocator_t *allocator,
	size_t size)
{
	return static_alloc (&((dn_allocator_static_t *)allocator)->_data, size);
}

static void *
static_vtable_realloc (
	dn_allocator_t *allocator,
	void *block,
	size_t size)
{
	return static_realloc (&((dn_allocator_static_t *)allocator)->_data, block, size);
}

static void
static_vtable_free (
	dn_allocator_t *allocator,
	void *block)
{
	static_free (&((dn_allocator_static_t *)allocator)->_data, block);
}

static void *
static_alloc (
	dn_allocator_static_data_t *data,
	size_t size)
{
	void *ptr = data->_ptr;
	void *new_ptr = (uint8_t *)ptr + DN_ALLOCATOR_ALIGN_SIZE (size + DN_ALLOCATOR_MEM_ALIGN8, DN_ALLOCATOR_MEM_ALIGN8);

	// Check if new memory address triggers OOM.
	if (!static_owns_ptr (data, new_ptr))
		return NULL;

	data->_ptr = new_ptr;

	*((size_t *)ptr) = size;
	return (uint8_t *)ptr + DN_ALLOCATOR_MEM_ALIGN8;
}

static void *
static_realloc (
	dn_allocator_static_data_t *data,
	void *block,
	size_t size)
{
	if (block && !static_owns_ptr (data, block))
		return NULL;

	void *ptr = data->_ptr;
	void *new_ptr = (uint8_t *)ptr + DN_ALLOCATOR_ALIGN_SIZE (size + DN_ALLOCATOR_MEM_ALIGN8, DN_ALLOCATOR_MEM_ALIGN8);

	// Check if new memory address triggers OOM.
	if (!static_owns_ptr (data, new_ptr))
		return NULL;

	data->_ptr = new_ptr;

	if (block)
		static_memcpy ((uint8_t *)ptr + DN_ALLOCATOR_MEM_ALIGN8, block, size);

	*((size_t *)ptr) = size;
	return (uint8_t *)ptr + DN_ALLOCATOR_MEM_ALIGN8;
}

static inline void
static_free (
	dn_allocator_static_data_t *data,
	void *block)
{
	DN_UNREFERENCED_PARAMETER (data);
	DN_UNREFERENCED_PARAMETER (block);

	// Sttaic buffer doesn't support free.
}

static inline bool
static_owns_ptr (
	dn_allocator_static_data_t *data,
	void *ptr)
{
	return (ptr >= data->_begin && ptr < data->_end);
}

static void *
static_memcpy (
	void *dst,
	void *src,
	size_t size)
{
	void *result = NULL;
	if (dst && src) {
		size_t *src_size = (size_t *)((uint8_t *)src - DN_ALLOCATOR_MEM_ALIGN8);
		if (src_size && src_size < (size_t *)src)
			result = memcpy (dst, src, size < *src_size ? size : *src_size);
	}

	return result;
}

static void *
static_dynamic_vtable_alloc (
	dn_allocator_t *allocator,
	size_t size)
{
	return static_dynamic_alloc (&((dn_allocator_static_dynamic_t *)allocator)->_data, size);
}

static void *
static_dynamic_vtable_realloc (
	dn_allocator_t *allocator,
	void *block,
	size_t size)
{
	return static_dynamic_realloc (&((dn_allocator_static_dynamic_t *)allocator)->_data, block, size);
}

static void
static_dynamic_vtable_free (
	dn_allocator_t *allocator,
	void *block)
{
	static_dynamic_free (&((dn_allocator_static_dynamic_t *)allocator)->_data, block);
}

static void *
static_dynamic_alloc (
	dn_allocator_static_dynamic_data_t *data,
	size_t size)
{
	void *result = NULL;

	result = static_alloc (data, size);
	if (!result)
		result = dn_malloc (size);

	return result;
}

static void *
static_dynamic_realloc (
	dn_allocator_static_dynamic_data_t *data,
	void *block,
	size_t size)
{
	// Check if ptr is owned by static buffer, if not, its own by heap.
	if (block && !static_owns_ptr (data, block))
		return dn_realloc (block, size);

	// Try realloc using static buffer.
	void *result = static_realloc (data, block, size);
	if (!result) {
		// Static buffer OOM, fallback to heap.
		result = dn_malloc (size);

		// Copy data from static buffer to allocated heap memory.
		if (block && result)
			result = static_memcpy (result, block, size);
	}

	return result;
}

static void
static_dynamic_free (
	dn_allocator_static_dynamic_data_t *data,
	void *block)
{
	if (static_owns_ptr (data, block))
		static_free (data, block);
	else
		dn_free (block);
}

dn_allocator_static_t *
dn_allocator_static_init (
	dn_allocator_static_t *allocator,
	void *block,
	size_t size)
{
	void *begin = DN_ALLOCATOR_ALIGN_PTR_TO (block, DN_ALLOCATOR_MEM_ALIGN8);
	void *end = (uint8_t *)begin + size - ((uint8_t *)begin - (uint8_t *)block);

	if (end < begin)
		return NULL;

	allocator->_data._begin = begin;
	allocator->_data._ptr = begin;
	allocator->_data._end = end;

	allocator->_vtable = &static_vtable;

	return allocator;
}

dn_allocator_static_t *
dn_allocator_static_reset(dn_allocator_static_t *allocator)
{
	allocator->_data._ptr = allocator->_data._begin;
	return allocator;
}

dn_allocator_static_dynamic_t *
dn_allocator_static_dynamic_reset(dn_allocator_static_dynamic_t *allocator)
{
	allocator->_data._ptr = allocator->_data._begin;
	return allocator;
}

dn_allocator_static_dynamic_t *
dn_allocator_static_dynamic_init (
	dn_allocator_static_dynamic_t *allocator,
	void *block,
	size_t size)
{
	void *begin = DN_ALLOCATOR_ALIGN_PTR_TO (block, DN_ALLOCATOR_MEM_ALIGN8);
	void *end = (uint8_t *)begin + size - ((uint8_t *)begin - (uint8_t *)block);

	if (end < begin)
		return NULL;

	allocator->_data._begin = begin;
	allocator->_data._ptr = begin;
	allocator->_data._end = end;

	allocator->_vtable = &static_dynamic_vtable;

	return allocator;
}
