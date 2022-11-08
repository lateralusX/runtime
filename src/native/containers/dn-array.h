
#ifndef __DN_ARRAY_H__
#define __DN_ARRAY_H__

#include "dn-utils.h"

typedef struct _dn_array_t dn_array_t;
struct _dn_array_t {
	char *data;
	int32_t len;
};

dn_array_t *
dn_array_new (
	bool zero_terminated,
	bool clear_,
	uint32_t element_size);

dn_array_t *
dn_array_sized_new (
	bool zero_terminated,
	bool clear_,
	uint32_t element_size,
	uint32_t reserved_size);

char *
dn_array_free (
	dn_array_t *array,
	bool free_segment);

void
dn_array_free_segment (char *segment);

dn_array_t *
dn_array_append_vals (
	dn_array_t *array,
	const void *data,
	uint32_t len);

dn_array_t *
dn_array_insert_vals (
	dn_array_t *array,
	uint32_t index_,
	const void *data,
	uint32_t len);

dn_array_t *
dn_array_remove_index (
	dn_array_t *array,
	uint32_t index_);

dn_array_t *
dn_array_remove_index_fast (
	dn_array_t *array,
	uint32_t index_);

void
dn_array_set_size (
	dn_array_t *array,
	int32_t length);

uint32_t
dn_array_capacity (dn_array_t *array);

#define dn_array_append_val(a,v) \
	(dn_array_append_vals((a),&(v),1))

#define dn_array_insert_val(a,i,v) \
	(dn_array_insert_vals((a),(i),&(v),1))

#define dn_array_index(a,t,i) \
	*(t*)(((a)->data) + sizeof(t) * (i))

#endif /* __DN_ARRAY_H__ */
