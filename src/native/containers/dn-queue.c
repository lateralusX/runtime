#include "dn-queue.h"

dn_queue_t *
_dn_queue_alloc (
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func)
{
	dn_queue_t *queue = (dn_queue_t *)dn_allocator_alloc (allocator, sizeof (dn_queue_t));
	if (!_dn_queue_init (queue, allocator, dispose_func)) {
		dn_allocator_free (allocator, queue);
		return NULL;
	}

	return queue;
}

bool
_dn_queue_init (
	dn_queue_t *queue,
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func)
{
	if (DN_UNLIKELY (!queue))
		return false;

	memset (queue, 0, sizeof(dn_queue_t));
	dn_list_custom_init (&queue->_internal.list, allocator, dispose_func);

	return true;
}

void
dn_queue_free (dn_queue_t *queue)
{
	if (queue) {
		dn_allocator_t *allocator = queue->_internal.list._internal._allocator;
		dn_list_dispose (&queue->_internal.list);
		dn_allocator_free (allocator, queue);
	}
}

void
dn_queue_dispose (dn_queue_t *queue)
{
	if (DN_UNLIKELY(!queue))
		return;

	dn_list_dispose (&queue->_internal.list);
}
