
#ifndef __DN_QUEUE_EX_H__
#define __DN_QUEUE_EX_H__

#include "dn-utils.h"
#include "dn-queue.h"

#define DN_QUEUE_EX_FOREACH_BEGIN(queue,var_type,var_name) do { \
	var_type var_name; \
	for (dn_list_t *__it##var_name = dn_list_first (queue->head); __it##var_name; __it##var_name = dn_list_next (__it##var_name)) { \
		var_name = (var_type)__it##var_name->data;

#define DN_QUEUE_EX_FOREACH_RBEGIN(queue,var_type,var_name) do { \
	var_type var_name; \
	for (dn_list_t *__it##var_name = dn_list_last (queue->head); __it##var_name; __it##var_name = dn_list_previous (__it##var_name)) { \
		var_name = (var_type)__it##var_name->data;

#define DN_QUEUE_EX_FOREACH_END \
		} \
	} while (0)

dn_queue_t *
dn_queue_ex_alloc (void);

void
dn_queue_ex_free (dn_queue_t **queue);

void
dn_queue_ex_for_each_free (
	dn_queue_t **queue,
	dn_free_func_t func);

static inline void *
dn_queue_ex_front (dn_queue_t *queue)
{
	if (!queue || !queue->head)
		return NULL;
	return queue->head->data;
}

static inline void *
dn_queue_ex_back (dn_queue_t *queue)
{
	if (!queue || !queue->tail)
		return NULL;
	return queue->tail->data;
}

static inline bool
dn_queue_ex_push_front (
	dn_queue_t *queue,
	void *data)
{
	dn_queue_push_head (queue, data);
	return true;
}

static inline void
dn_queue_ex_pop_front (dn_queue_t *queue)
{
	dn_queue_pop_head (queue);
}

static inline bool
dn_queue_ex_push_back (
	dn_queue_t *queue,
	void *data)
{
	dn_queue_push_tail (queue, data);
	return true;
}

static inline void
dn_queue_ex_pop_back (dn_queue_t *queue)
{
	DN_UNREFERENCED_PARAMETER (queue);
}

void
dn_queue_ex_clear (dn_queue_t *queue);

static inline bool
dn_queue_ex_empty (dn_queue_t *queue)
{
	return dn_queue_is_empty (queue);
}

static inline uint32_t
dn_queue_size (dn_queue_t *queue)
{
	if (!queue)
		return 0;
	return queue->length;
}

void
dn_queue_ex_clear (dn_queue_t *queue);

void
dn_queue_ex_for_each_clear (
	dn_queue_t *queue,
	dn_free_func_t func);

#endif /* __DN_QUEUE_EX_H__ */
