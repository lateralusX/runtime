
#ifndef __DN_PTR_ARRAY_H__
#define __DN_PTR_ARRAY_H__

#include "dn-utils.h"

typedef struct _dn_ptr_array_t dn_ptr_array_t;
struct _dn_ptr_array_t {
	void **data;
	uint32_t len;
};

dn_ptr_array_t *
dn_ptr_array_new (void);

dn_ptr_array_t *
dn_ptr_array_sized_new (uint32_t reserved_size);

bool
dn_ptr_array_add (
	dn_ptr_array_t *array,
	void *data);

bool
dn_ptr_array_remove (
	dn_ptr_array_t *array,
	void *data);

void *
dn_ptr_array_remove_index (
	dn_ptr_array_t *array,
	uint32_t index);

bool
dn_ptr_array_remove_fast (
	dn_ptr_array_t *array,
	void *data);

void *
dn_ptr_array_remove_index_fast (
	dn_ptr_array_t *array,
	uint32_t index);

void
dn_ptr_array_sort (
	dn_ptr_array_t *array,
	dn_compare_func_t compare_func);

void
dn_ptr_array_set_size (
	dn_ptr_array_t *array,
	int32_t length);

void **
dn_ptr_array_free (
	dn_ptr_array_t *array,
	bool free_seg);

void
dn_ptr_array_free_segment (void **segment);

void
dn_ptr_array_foreach (
	dn_ptr_array_t *array,
	dn_func_t func,
	void *user_data);

uint32_t
dn_ptr_array_capacity (dn_ptr_array_t *array);

bool
dn_ptr_array_find (
	dn_ptr_array_t *array,
	const void *needle,
	uint32_t *index);

#define dn_ptr_array_index(array,index) \
	(array)->data[(index)]

#endif /* __DN_PTR_ARRAY_H__ */
