
#ifndef __DN_BYTE_ARRAY_EX_H__
#define __DN_BYTE_ARRAY_EX_H__

#include "dn-utils.h"
#include "dn-byte-array.h"

#define DN_BYTE_ARRAY_EX_FOREACH_BEGIN(array,var_type,var_name) do { \
	var_type var_name; \
	for (int32_t __i_##var_name = 0; __i_##var_name < array->len; ++__i_##var_name) { \
		var_name = dn_array_index (array, var_type, __i_##var_name);

#define DN_BYTE_ARRAY_EX_FOREACH_RBEGIN(array,var_type,var_name) do { \
	var_type var_name; \
	for (int32_t __i_##var_name = array->len; __i_##var_name > 0; --__i_##var_name) { \
		var_name = dn_array_index (array, var_type, __i_##var_name - 1);

#define DN_BYTE_ARRAY_EX_FOREACH_END \
		} \
	} while (0)

dn_byte_array_t *
dn_byte_array_ex_alloc (void);

dn_byte_array_t *
dn_byte_array_ex_alloc_capacity(uint32_t capacity);

void
dn_byte_array_ex_free (dn_byte_array_t **array);

#define dn_byte_array_ex_push_back(a,v) \
	(dn_array_append_vals((dn_array_t *)(a),&(v),1) != NULL)

#define dn_byte_array_ex_erase(a,i) \
	(dn_array_remove_index ((dn_array_t *)(a), i) != NULL)

#define dn_byte_array_ex_clear(a) \
	(dn_array_set_size ((dn_array_t *)(a), 0))

#define dn_byte_array_ex_empty(a) \
	(a == NULL || a->len == 0)

#define dn_byte_array_ex_data(a) \
	((uint8_t *)(a->data))

#define dn_byte_array_ex_size(a) \
	(uint32_t)(a->len)

#endif /* __DN_BYTE_ARRAY_EX_H__ */
