
#ifndef __DN_QUEUE_H__
#define __DN_QUEUE_H__

#include "dn-list-ex.h"
#include "dn-utils.h"

typedef struct _dn_queue_t dn_queue_t;

struct _dn_queue_t {
	dn_list_t *head;
	dn_list_t *tail;
	uint32_t length;
};

dn_queue_t *
dn_queue_new (void);

void
dn_queue_free (dn_queue_t *queue);

void *
dn_queue_pop_head (dn_queue_t *queue);

void
dn_queue_push_head (
	dn_queue_t *queue,
	void *data);

void
dn_queue_push_tail (
	dn_queue_t *queue,
	void *data);

bool
dn_queue_is_empty (dn_queue_t *queue);

void
dn_queue_foreach (
	dn_queue_t *queue,
	dn_func_t func,
	void *user_data);

#endif /* __DN_QUEUE_H__ */
