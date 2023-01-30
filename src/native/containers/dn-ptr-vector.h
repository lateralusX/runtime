// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#ifndef __DN_PTR_VECTOR_H__
#define __DN_PTR_VECTOR_H__

#include "dn-utils.h"
#include "dn-vector.h"

DN_DEFINE_VECTOR_T (ptr, void *)

#define DN_PTR_VECTOR_FOREACH_BEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	DN_ASSERT (dn_ptr_vector_check_version (vector)); \
	DN_ASSERT (sizeof (var_type) == vector->_internal._element_size); \
	for (uint32_t __i_ ## var_name = 0; __i_ ## var_name < (vector)->size; ++__i_ ## var_name) { \
		var_name = (var_type)*dn_ptr_vector_index (vector, __i_##var_name);

#define DN_PTR_VECTOR_FOREACH_RBEGIN(vector,var_type,var_name) do { \
	var_type var_name; \
	DN_ASSERT (dn_ptr_vector_check_version (vector)); \
	DN_ASSERT (sizeof (var_type) == vector->_internal._element_size); \
	for (uint32_t __i_ ## var_name = (vector)->size; __i_ ## var_name > 0; --__i_ ## var_name) { \
		var_name = (var_type)*dn_ptr_vector_index (vector, __i_ ## var_name - 1);

#define DN_PTR_VECTOR_FOREACH_END \
		} \
	} while (0)

#endif /* __DN_PTR_VECTOR_H__ */
