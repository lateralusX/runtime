
#ifndef __DN_QUEUE_H__
#define __DN_QUEUE_H__

#include "dn-utils.h"
#include "dn-allocator.h"
#include "dn-list.h"

typedef struct _dn_queue_t dn_queue_t;

struct _dn_queue_t {
	uint32_t size;
	struct {
		dn_list_t list;
	} _internal;
};

dn_queue_t *
_dn_queue_alloc (
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func);

bool
_dn_queue_init (
	dn_queue_t *queue,
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func);

static inline dn_queue_t *
dn_queue_custom_alloc (
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func)
{
	return _dn_queue_alloc (allocator, dispose_func);
}

static inline dn_queue_t *
dn_queue_alloc (void)
{
	return _dn_queue_alloc (DN_DEFAULT_ALLOCATOR, NULL);
}

void
dn_queue_free (dn_queue_t *queue);

static inline bool
dn_queue_custom_init (
	dn_queue_t *queue,
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func)
{
	return _dn_queue_init (queue, allocator, dispose_func);
}

static inline bool
dn_queue_init (dn_queue_t *queue)
{
	return _dn_queue_init (queue, DN_DEFAULT_ALLOCATOR, NULL);
}

void
dn_queue_dispose (dn_queue_t *queue);

static inline void **
dn_queue_front (dn_queue_t *queue)
{
	if (!queue || queue->size == 0)
		return NULL;

	return dn_list_front (&queue->_internal.list);
}

#define dn_queue_front_t(queue, type) \
	(type *)dn_queue_front ((queue))

static inline void **
dn_queue_back (dn_queue_t *queue)
{
	if (!queue || queue->size == 0)
		return NULL;

	return dn_list_back (&queue->_internal.list);
}

#define dn_queue_back_t(queue, type) \
	(type *)dn_queue_back ((queue))

static inline bool
dn_queue_empty (dn_queue_t *queue)
{
	return queue == NULL || queue->size == 0;
}

static inline uint32_t
dn_queue_size (dn_queue_t *queue)
{
	return queue ? queue->size : 0;
}

static inline bool
dn_queue_push (
	dn_queue_t *queue,
	void *data)
{
	bool result = false;
	if (!queue)
		return result;

	result = dn_list_push_back (&queue->_internal.list, data);
	if (result)
		queue->size ++;

	return result;
}

static inline void
dn_queue_pop (dn_queue_t *queue)
{
	if (queue) {
		dn_list_pop_front (&queue->_internal.list);
		queue->size --;
	}
}

static inline void
dn_queue_clear (dn_queue_t *queue)
{
	if (queue) {
		dn_list_clear (&queue->_internal.list);
		queue->size = 0;
	}
}

#endif /* __DN_QUEUE_H__ */
