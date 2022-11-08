/*
 * Pointer Array
 *
 * Author:
 *   Aaron Bockover (abockover@novell.com)
 *   Gonzalo Paniagua Javier (gonzalo@novell.com)
 *   Jeffrey Stedfast (fejj@novell.com)
 *
 * (C) 2006,2011 Novell, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "dn-ptr-array-ex.h"
#include "dn-malloc.h"

typedef struct _dn_ptr_array_priv_t {
	void **data;
	uint32_t len;
	uint32_t size;
	dn_allocator_t *allocator;
} dn_ptr_array_priv_t;

// Until C11 support, use typedef expression for static assertion.
#define _DN_STATIC_ASSERT_UNQIUE_TYPEDEF0(line) __dn_static_assert_ ## line ## _t
#define _DN_STATIC_ASSERT_UNQIUE_TYPEDEF(line) _DN_STATIC_ASSERT_UNQIUE_TYPEDEF0(line)
#define _DN_STATIC_ASSERT(expr) typedef char _DN_STATIC_ASSERT_UNQIUE_TYPEDEF(__LINE__)[(expr) != 0]

// Verify that headers defined max size of dn_ptr_array_priv_t is inline with actual type size.
_DN_STATIC_ASSERT (DN_PTR_ARRAY_EX_MAX_TYPE_ALLOC_SIZE >= DN_ALLOCATOR_ALIGN_SIZE (sizeof (dn_ptr_array_priv_t), DN_ALLOCATOR_MEM_ALIGN8));

static bool
dn_ptr_array_grow (
	dn_ptr_array_priv_t *array,
	uint32_t length)
{
	uint64_t new_length = array->len + length;

	if (new_length <= (uint64_t)(array->size))
		return true;

	if (DN_UNLIKELY (new_length > (uint64_t)(UINT32_MAX)))
		return false;

	uint32_t new_size = 1;
	while(new_size < (uint32_t)new_length)
		new_size <<= 1;

	new_size = new_size > 16 ? new_size : 16;

	size_t realloc_size;
	if (DN_UNLIKELY (!dn_safe_size_t_multiply (sizeof(void *), new_size, &realloc_size)))
		return false;

	void **data = (void **)(array->allocator ?
		dn_allocator_realloc (array->allocator, array->data, realloc_size) :
		dn_realloc (array->data, realloc_size));

	if (DN_UNLIKELY (!data && realloc_size != 0))
		return false;

	array->size = new_size;
	array->data = data;

	return array->data != NULL;
}

dn_ptr_array_t *
dn_ptr_array_new (void)
{
	return dn_ptr_array_ex_alloc_capacity (0);
}

dn_ptr_array_t *
dn_ptr_array_sized_new (uint32_t reserved_size)
{
	return dn_ptr_array_ex_alloc_capacity (reserved_size);
}

void **
dn_ptr_array_free (
	dn_ptr_array_t *array,
	bool free_seg)
{
	dn_ptr_array_priv_t *array_priv = (dn_ptr_array_priv_t*)array;
	void **data = NULL;

	if (array_priv == NULL)
		return NULL;

	if(free_seg) {
		array_priv->allocator ?
			dn_allocator_free (array_priv->allocator, array_priv->data) :
			dn_free (array_priv->data);
	} else {
		data = array_priv->data;
	}

	array_priv->allocator ?
		dn_allocator_free (array_priv->allocator, array_priv) :
		dn_free (array_priv);

	return data;
}

void
dn_ptr_array_free_segment (void **segment)
{
	dn_free (segment);
}

void
dn_ptr_array_set_size (
	dn_ptr_array_t *array,
	int32_t length)
{
	if (array == NULL || length < 0)
		return;

	if((uint32_t)length > array->len) {
		if (DN_UNLIKELY (!dn_ptr_array_grow((dn_ptr_array_priv_t *)array, (uint32_t)length)))
			return;

		// array->len < new allocated memory, calculations won't overflow/underflow.
		memset(array->data + array->len, 0, ((uint32_t)length - array->len) * sizeof(void *));
	}

	array->len = (uint32_t)length;
	return;
}

bool
dn_ptr_array_add (
	dn_ptr_array_t *array,
	void *data)
{
	if (array == NULL)
		return false;

	if (DN_UNLIKELY (!dn_ptr_array_grow ((dn_ptr_array_priv_t *)array, 1)))
		return false;

	/* Calculation won't overflow, already checked in dn_ptr_array_grow */
	array->data[array->len++] = data;

	return true;
}

void *
dn_ptr_array_remove_index (
	dn_ptr_array_t *array,
	uint32_t index)
{
	void *removed_node;

	if (array == NULL || index >= array->len)
		return NULL;

	removed_node = array->data [index];
	
	/* index < array->len, calculations won't underflow/overflow */
	if (index != array->len - 1)
		memmove (array->data + index, array->data + index + 1, (array->len - index - 1) * sizeof(void *));

	array->len--;
	array->data [array->len] = NULL;

	return removed_node;
}

void *
dn_ptr_array_remove_index_fast (
	dn_ptr_array_t *array,
	uint32_t index)
{
	void *removed_node;

	if (array == NULL || index >= array->len)
		return NULL;

	removed_node = array->data [index];

	/* index < array->len, alculations won't underflow/overflow */
	if(index != array->len - 1)
		memmove (array->data + index, array->data + array->len - 1, sizeof(void *));

	array->len--;
	array->data[array->len] = NULL;

	return removed_node;
}

bool
dn_ptr_array_remove (
	dn_ptr_array_t *array,
	void *data)
{
	if (array == NULL)
		return false;

	for (uint32_t i = 0; i < array->len; i++) {
		if (array->data [i] == data) {
			if (dn_ptr_array_remove_index(array, i))
				return true;
		}
	}

	return false;
}

bool
dn_ptr_array_remove_fast (
	dn_ptr_array_t *array,
	void *data)
{
	if (array == NULL)
		return false;

	for (uint32_t i = 0; i < array->len; i++) {
		if (array->data [i] == data) {
			array->len--;
			if (array->len > 0)
				array->data [i] = array->data [array->len];
			else
				array->data [i] = NULL;
			return true;
		}
	}

	return false;
}

void
dn_ptr_array_foreach (
	dn_ptr_array_t *array,
	dn_func_t func,
	void *user_data)
{
	if (array == NULL)
		return;

	for(uint32_t i = 0; i < array->len; i++) {
		func (dn_ptr_array_index (array, i), user_data);
	}
}

// Make sure assumption holds for qsort callback.
_DN_STATIC_ASSERT (sizeof (int) == sizeof (int32_t));

void
dn_ptr_array_sort (
	dn_ptr_array_t *array,
	dn_compare_func_t compare)
{
	if (array == NULL || array->len < 2)
		return;
	qsort ((void *)array->data, array->len, sizeof(void *), (int (DN_CALLBACK_CALLTYPE *)(const void *, const void *))compare);
}

uint32_t
dn_ptr_array_capacity (dn_ptr_array_t *array)
{
	return ((dn_ptr_array_priv_t *)array)->size;
}

bool
dn_ptr_array_find (
	dn_ptr_array_t *array,
	const void *needle,
	uint32_t *index)
{
	if (array == NULL)
		return false;

	for (uint32_t i = 0; i < array->len; i++) {
		if (array->data [i] == needle) {
			if (index)
				*index = i;
			return true;
		}
	}

	return false;
}

uint32_t
dn_ptr_array_ex_buffer_capacity (size_t buffer_size)
{
	// Estimate maximum array capacity for buffer size.
	size_t max_capacity = (buffer_size - DN_ALLOCATOR_ALIGN_SIZE (sizeof (dn_ptr_array_priv_t), DN_ALLOCATOR_MEM_ALIGN8) - 32) / sizeof (void *);
	if (DN_UNLIKELY (max_capacity > buffer_size || max_capacity > (size_t)UINT32_MAX))
		return 0;

	// Adjust to heuristics in dn_ptr_array_grow.
	uint32_t capacity = 1;
	while(capacity < (uint32_t)max_capacity)
		capacity <<= 1;

	return (uint32_t)(capacity >> 1);
}

dn_ptr_array_t *
dn_ptr_array_ex_alloc_custom (dn_allocator_t *allocator)
{
	return dn_ptr_array_ex_alloc_capacity_custom (allocator, 0);
}

dn_ptr_array_t *
dn_ptr_array_ex_alloc_capacity_custom (
	dn_allocator_t *allocator,
	uint32_t capacity)
{
	dn_ptr_array_priv_t *array = (dn_ptr_array_priv_t *)(allocator ?
		dn_allocator_alloc (allocator, sizeof (dn_ptr_array_priv_t)) :
		dn_malloc (sizeof (dn_ptr_array_priv_t)));

	if (DN_UNLIKELY (!array))
		return NULL;

	array->data = NULL;
	array->len = 0;
	array->size = 0;
	array->allocator = allocator;

	if(capacity > 0) {
		if (DN_UNLIKELY (!dn_ptr_array_grow (array, capacity))) {
			dn_free (array);
			return NULL;
		}
	}

	return (dn_ptr_array_t *)array;
}

void
dn_ptr_array_ex_free (dn_ptr_array_t **array)
{
	if (array) {
		dn_ptr_array_free (*array, true);
		*array = NULL;
	}
}

void
dn_ptr_array_ex_for_each_free (
	dn_ptr_array_t **array,
	dn_free_func_t func)
{
	if (array) {
		if (*array && func) {
			for (uint32_t i = 0; i < (*array)->len; ++i)
				func ((*array)->data [i]);
		}

		dn_ptr_array_free (*array, true);
		*array = NULL;
	}
}

void
dn_ptr_array_ex_for_each_clear (
	dn_ptr_array_t *array,
	dn_free_func_t func)
{
	if (array) {
		if (func) {
			for (uint32_t i = 0; i < array->len; ++i)
				func (array->data [i]);
		}

		dn_ptr_array_set_size (array, 0);
	}
}
