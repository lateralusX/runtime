#ifndef __DN_BYTE_ARRAY_H__
#define __DN_BYTE_ARRAY_H__

#include "dn-utils.h"
#include "dn-array.h"

typedef struct _dn_byte_array_t dn_byte_array_t;

struct _dn_byte_array_t {
	uint8_t *data;
	int32_t len;
};

dn_byte_array_t *
dn_byte_array_new (void);

uint8_t *
dn_byte_array_free (
	dn_byte_array_t *array,
	bool free_seg);

void
dn_byte_array_free_segment (uint8_t *segment);

dn_byte_array_t *
dn_byte_array_append (
	dn_byte_array_t *array,
	const uint8_t *data,
	uint32_t len);

void
dn_byte_array_set_size (
	dn_byte_array_t *array,
	int32_t length);

uint32_t
dn_byte_array_capacity (dn_byte_array_t *array);

#endif /* __DN_BYTE_ARRAY_H__ */
