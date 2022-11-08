
#ifndef __DN_PTR_ARRAY_EX_H__
#define __DN_PTR_ARRAY_EX_H__

#include "dn-utils.h"
#include "dn-ptr-array.h"
#include "dn-allocator.h"

#define DN_PTR_ARRAY_EX_FOREACH_BEGIN(array,var_type,var_name) do { \
	var_type var_name; \
	for (uint32_t __i_##var_name = 0; __i_##var_name < array->len; ++__i_##var_name) { \
		var_name = (var_type)dn_ptr_array_index (array, __i_##var_name);

#define DN_PTR_ARRAY_EX_FOREACH_RBEGIN(array,var_type,var_name) do { \
	var_type var_name; \
	for (uint32_t __i_##var_name = array->len; __i_##var_name > 0; --__i_##var_name) { \
		var_name = (var_type)dn_ptr_array_index (array, __i_##var_name - 1);

#define DN_PTR_ARRAY_EX_FOREACH_END \
		} \
	} while (0)

#define DN_PTR_ARRAY_EX_MAX_TYPE_ALLOC_SIZE 32
#define DN_PTR_ARRAY_EX_MAX_ADDITIONAL_ALLOC_SIZE ((DN_ALLOCATOR_MAX_ALIGNMENT * 2) + DN_PTR_ARRAY_EX_MAX_TYPE_ALLOC_SIZE)

uint32_t
dn_ptr_array_ex_buffer_capacity (size_t buffer);

dn_ptr_array_t *
dn_ptr_array_ex_alloc_custom (dn_allocator_t *allocator);

dn_ptr_array_t *
dn_ptr_array_ex_alloc_capacity_custom (
	dn_allocator_t *allocator,
	uint32_t capacity);

#define dn_ptr_array_ex_alloc() dn_ptr_array_ex_alloc_custom (NULL)
#define dn_ptr_array_ex_alloc_capacity(capacity) dn_ptr_array_ex_alloc_capacity_custom (NULL, capacity)

void
dn_ptr_array_ex_free (dn_ptr_array_t **array);

void
dn_ptr_array_ex_for_each_free (
	dn_ptr_array_t **array,
	dn_free_func_t func);

#define dn_ptr_array_ex_push_back(a,v) \
	dn_ptr_array_add (a, v)

#define dn_ptr_array_ex_erase(a,v) \
	dn_ptr_array_remove (a, v)

static inline void
dn_ptr_array_ex_clear (dn_ptr_array_t *array)
{
	dn_ptr_array_set_size (array, 0);
}

void
dn_ptr_array_ex_for_each_clear (
	dn_ptr_array_t *array,
	dn_free_func_t func);

#define dn_ptr_array_ex_empty(a) \
	(a == NULL || a->len == 0)

#define dn_ptr_array_ex_data(a,t) \
	((t)(a->data))

#define dn_ptr_array_ex_size(a) \
	(uint32_t)a->len

#define DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE ((sizeof (void *) * 256) + DN_PTR_ARRAY_EX_MAX_ADDITIONAL_ALLOC_SIZE)
#define DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_CAPACITY dn_ptr_array_ex_buffer_capacity (DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE)
#define DN_PTR_ARRAY_EX_LOCAL_ALLOCATOR(var_name, buffer_size) DN_ALLOCATOR_STATIC_DYNAMIC (var_name, buffer_size)

#endif /* __DN_PTR_ARRAY_EX_H__ */
