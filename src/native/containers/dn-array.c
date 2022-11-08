/*
 * Arrays
 *
 * Author:
 *   Chris Toshok (toshok@novell.com)
 *
 * (C) 2006 Novell, Inc.
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
#include "dn-array-ex.h"
#include "dn-malloc.h"

#define INITIAL_CAPACITY 16

#define element_offset(p,i) \
	((p)->array.data + (i) * (p)->element_size)

#define element_length(p,i) \
	((i) * (p)->element_size)

typedef struct {
	dn_array_t array;
	bool clear_;
	uint32_t element_size;
	bool zero_terminated;
	uint32_t capacity;
} dn_array_priv_t;

static bool
ensure_capacity (
	dn_array_priv_t *priv,
	uint32_t capacity)
{
	uint64_t new_capacity;

	if (capacity <= (uint64_t)(priv->capacity))
		return true;

	new_capacity = (capacity + (capacity >> 1) + 63) & ~63;

	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return false;

	size_t realloc_size;
	if (DN_UNLIKELY (!dn_safe_size_t_multiply (priv->element_size, (size_t)new_capacity, &realloc_size)))
		return false;

	char *data = (char *)dn_realloc (priv->array.data, realloc_size);
	if (DN_UNLIKELY (!data && realloc_size != 0))
		return false;

	priv->array.data = data;

	if (priv->clear_ && priv->array.data) {
		// Checks already verified that element_offset won't overflow.
		// new_capacity > priv->capacity, so new_capacity - priv->capacity won't underflow.
		// dn_safe_size_t_multiply already verified element_length won't overflow.
		memset (element_offset (priv, priv->capacity), 0, element_length (priv, (uint32_t)new_capacity - priv->capacity));
	}

	// Overflow already checked.
	priv->capacity = (uint32_t)new_capacity;

	return priv->array.data != NULL;
}

dn_array_t *
dn_array_new (
	bool zero_terminated,
	bool clear_,
	uint32_t element_size)
{
	dn_array_priv_t *rv = dn_new0 (dn_array_priv_t, 1);
	if (DN_UNLIKELY (!rv))
		return NULL;

	rv->zero_terminated = zero_terminated;
	rv->clear_ = clear_;
	rv->element_size = element_size;

	if (DN_UNLIKELY (!ensure_capacity (rv, INITIAL_CAPACITY))) {
		dn_free (rv);
		return NULL;
	}

	return (dn_array_t*)rv;
}

dn_array_t *
dn_array_sized_new (
	bool zero_terminated,
	bool clear_,
	uint32_t element_size,
	uint32_t reserved_size)
{
	dn_array_priv_t *rv = dn_new0 (dn_array_priv_t, 1);
	if (DN_UNLIKELY (!rv))
		return NULL;

	rv->zero_terminated = zero_terminated;
	rv->clear_ = clear_;
	rv->element_size = element_size;

	if (DN_UNLIKELY (!ensure_capacity (rv, reserved_size))) {
		dn_free (rv);
		return NULL;
	}

	return (dn_array_t*)rv;
}

char *
dn_array_free (
	dn_array_t *array,
	bool free_segment)
{
	char *rv = NULL;

	if (array == NULL)
		return NULL;

	if (free_segment)
		dn_free (array->data);
	else
		rv = array->data;

	dn_free (array);

	return rv;
}

void
dn_array_free_segment (char *segment)
{
	dn_free (segment);
}

dn_array_t *
dn_array_append_vals (
	dn_array_t *array,
	const void *data,
	uint32_t len)
{
	dn_array_priv_t *priv = (dn_array_priv_t*)array;

	if (array == NULL)
		return NULL;

	uint32_t extra = (priv->zero_terminated ? 1 : 0);
	uint64_t new_capacity = (uint64_t)priv->array.len + (uint64_t)len + (uint64_t)extra;
	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return NULL;
	
	if (DN_UNLIKELY (!ensure_capacity (priv, (uint32_t)new_capacity)))
		return NULL;

	/* ensure_capacity already verified element_offset and element_length won't overflow. */
	memmove (element_offset (priv, priv->array.len), data, element_length (priv, len));

	// Overflowed already checked.
	priv->array.len += len;

	if (priv->zero_terminated)
		/* ensure_capacity already verified element_offset won't overflow. */
		memset (element_offset (priv, priv->array.len), 0, priv->element_size);

	return array;
}

dn_array_t *
dn_array_insert_vals (
	dn_array_t *array,
	uint32_t index_,
	const void *data,
	uint32_t len)
{
	dn_array_priv_t *priv = (dn_array_priv_t*)array;

	if (array == NULL)
		return NULL;

	uint32_t extra = (priv->zero_terminated ? 1 : 0);
	uint64_t new_capacity = (uint64_t)array->len + (uint64_t)len + (uint64_t)extra;
	if (DN_UNLIKELY (new_capacity > (uint64_t)(UINT32_MAX)))
		return NULL;

	if (DN_UNLIKELY (!ensure_capacity (priv, (uint32_t)new_capacity)))
		return NULL;

	uint64_t insert_offset = index_ + len;
	int32_t size_to_move = array->len - (int32_t)index_;
	if (DN_UNLIKELY (insert_offset > new_capacity || size_to_move > array->len || size_to_move < 0))
		return NULL;

	/* first move the existing elements out of the way */
	/* element_offset won't overflow since insert_offset and index_ is smaller than new_capacity already checked in ensure_capacity */
	/* element_length won't overflow since size_to_move is smaller than new_capacity already checked in ensure_capacity */
	/* element_length won't underflow since size_to_move is already verfied to be smaller or equal to array->len*/
	memmove (element_offset (priv, insert_offset), element_offset (priv, index_), element_length (priv, size_to_move));

	/* then copy the new elements into the array */
	/* element_offset won't overflow since index_ is smaller than new_capacity already checked in encure_capacity */
	/* element_length won't overflow since len is included in new_capacity already checked in encure_capacity */
	memmove (element_offset (priv, index_), data, element_length (priv, len));

	// Overflow already checked.
	array->len += len;

	if (priv->zero_terminated)
		/* element_offset won't overflow since new array len is already checked in ensure_capacity. */
		memset (element_offset (priv, priv->array.len), 0, priv->element_size);

	return array;
}

dn_array_t *
dn_array_remove_index (
	dn_array_t *array,
	uint32_t index_)
{
	dn_array_priv_t *priv = (dn_array_priv_t*)array;

	if (array == NULL)
		return NULL;

	uint64_t insert_offset = index_ + 1;
	int32_t size_to_move = array->len - (int32_t)index_;
	if (DN_UNLIKELY (insert_offset > priv->capacity || size_to_move < 0))
		return NULL;
	
	/* element_offset won't overflow since insert_offset and index_ is smaller than current capacity */
	/* element_length won't overflow since size_to_move is smaller than current capacity */
	/* element_length won't underflow since size_to_move is already verfied to be 0 or larger */
	memmove (element_offset (priv, index_), element_offset (priv, insert_offset), element_length (priv, size_to_move));

	array->len --;

	if (priv->zero_terminated)
		/* element_offset won't overflow since priv->array.len is smaller than current capacity */
		memset (element_offset (priv, priv->array.len), 0, priv->element_size);

	return array;
}

dn_array_t *
dn_array_remove_index_fast (
	dn_array_t *array,
	uint32_t index_)
{
	dn_array_priv_t *priv = (dn_array_priv_t*)array;

	if (array == NULL)
		return NULL;

	if (DN_UNLIKELY (array->len <= 0 || (int32_t)index_ > array->len))
		return NULL;

	/* element_offset won't overflow since index_ is smaller than current capacity */
	/* element_offset won't overflow since priv->array.len - 1 is smaller than current capacity */
	/* priv->array.len - 1 won't underflow since array->len > 0 */
	memmove (element_offset (priv, index_), element_offset (priv, array->len - 1), element_length (priv, 1));

	array->len --;

	if (priv->zero_terminated)
		/* element_offset won't overflow since priv->array.len is smaller than current capacity */
		memset (element_offset (priv, priv->array.len), 0, priv->element_size);

	return array;
}

void
dn_array_set_size (
	dn_array_t *array,
	int32_t length)
{
	dn_array_priv_t *priv = (dn_array_priv_t*)array;

	if (array == NULL || length < 0)
		return;

	if ((uint32_t)length == priv->capacity)
		return;

	if ((uint32_t)length > priv->capacity)
		if (DN_UNLIKELY (!ensure_capacity (priv, (uint32_t)length)))
			return;

	array->len = length;
}

uint32_t
dn_array_capacity (dn_array_t *array)
{
	return ((dn_array_priv_t *)array)->capacity;
}

void
dn_array_ex_free(dn_array_t **array)
{
	if (array) {
		dn_array_free (*array, true);
		*array = NULL;
	}
}
