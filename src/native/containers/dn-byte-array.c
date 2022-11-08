/*
 * Arrays
 *
 * Author:
 *   Geoff Norton  (gnorton@novell.com)
 *
 * (C) 2010 Novell, Inc.
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
#include "dn-byte-array-ex.h"
#include "dn-malloc.h"

dn_byte_array_t *
dn_byte_array_new (void)
{
	return (dn_byte_array_t *)dn_array_new (false, true, 1);
}

uint8_t *
dn_byte_array_free (
	dn_byte_array_t *array,
	bool free_seg)
{
	return (uint8_t *)dn_array_free ((dn_array_t *)array, free_seg);
}

void
dn_byte_array_free_segment (uint8_t *segment)
{
	dn_free (segment);
}

dn_byte_array_t *
dn_byte_array_append (
	dn_byte_array_t *array,
	const uint8_t *data,
	uint32_t len)
{
	return (dn_byte_array_t *)dn_array_append_vals ((dn_array_t *)array, data, len);
}

void
dn_byte_array_set_size (
	dn_byte_array_t *array,
	int32_t length)
{
	dn_array_set_size ((dn_array_t*)array, length);
}

uint32_t
dn_byte_array_capacity (dn_byte_array_t *array)
{
	return dn_array_capacity ((dn_array_t *)array);
}

dn_byte_array_t *
dn_byte_array_ex_alloc (void)
{
	return (dn_byte_array_t *)dn_array_new (false, true, 1);
}

dn_byte_array_t *
dn_byte_array_ex_alloc_capacity (uint32_t capacity)
{
	return (dn_byte_array_t *)dn_array_sized_new (false, true, 1, capacity);
}

void
dn_byte_array_ex_free(dn_byte_array_t **array)
{
	if (array) {
		dn_byte_array_free (*array, true);
		*array = NULL;
	}
}
