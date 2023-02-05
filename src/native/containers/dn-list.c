#include "dn-list.h"

static dn_list_node_t *
list_new_node (
	dn_allocator_t *allocator,
	dn_list_node_t *prev,
	dn_list_node_t *next,
	void *data)
{
	dn_list_node_t *node = (dn_list_node_t *)dn_allocator_alloc (allocator, sizeof (dn_list_node_t));
	if (DN_UNLIKELY (!node))
		return NULL;

	node->data = data;
	node->prev = prev;
	node->next = next;

	if (prev)
		prev->next = node;
	if (next)
		next->prev = node;

	return node;
}

static dn_list_node_t*
list_unlink_node (dn_list_node_t *node)
{
	if (node->next)
		node->next->prev = node->prev;
	if (node->prev)
		node->prev->next = node->next;

	return node;
}


static dn_list_node_t *
list_insert_node_before (
	dn_allocator_t *allocator,
	dn_list_node_t *node,
	void *data)
{
	return list_new_node (allocator, node ? node->prev : NULL, node, data);
}

static dn_list_node_t *
list_insert_node_after (
	dn_allocator_t *allocator,
	dn_list_node_t *node,
	void *data)
{
	return list_new_node (allocator, node, node ? node->next : NULL, data);
}

static void
list_free_node (
	dn_allocator_t *allocator,
	dn_list_node_t *node)
{
	dn_allocator_free (allocator, node);
}

static void
list_dispose_node (
	dn_allocator_t *allocator,
	dn_func_t func,
	dn_list_node_t *node)
{
	if (node && func)
		func (node->data);
	list_free_node (allocator, node);
}

dn_list_t *
_dn_list_alloc (dn_allocator_t *allocator)
{
	dn_list_t *list = (dn_list_t *)dn_allocator_alloc (allocator, sizeof (dn_list_t));
	if (!_dn_list_init (list, allocator)) {
		dn_allocator_free (allocator, list);
		return NULL;
	}

	return list;
}

bool
_dn_list_init (
	dn_list_t *list,
	dn_allocator_t *allocator)
{
	if (DN_UNLIKELY (!list))
		return false;

	memset (list, 0, sizeof(dn_list_t));
	list->_internal._allocator = allocator;

	return true;
}

void
dn_list_custom_free (
	dn_list_t *list,
	dn_func_t func)
{
	if (list) {
		dn_list_custom_dispose (list, func);
		dn_allocator_free (list->_internal._allocator, list);
	}
}

void
dn_list_custom_dispose (
	dn_list_t *list,
	dn_func_t func)
{
	if (DN_UNLIKELY(!list))
		return;

	dn_list_node_t *current = list->head;
	while (current) {
		dn_list_node_t *next = current->next;
		list_dispose_node (list->_internal._allocator, func, current);
		current = next;
	}
}

uint32_t
dn_list_size (const dn_list_t *list)
{
	if (DN_UNLIKELY(!list))
		return 0;

	uint32_t size = 0;
	dn_list_node_t *nodes = list->head;

	while (nodes) {
		size ++;
		nodes = nodes->next;
	}

	return size;
}

void
dn_list_custom_clear (
	dn_list_t *list,
	dn_func_t func)
{
	if (DN_UNLIKELY(!list))
		return;

	dn_list_custom_dispose (list, func);

	list->head = NULL;
	list->tail = NULL;
}

dn_list_result_t
dn_list_insert (
	dn_list_it_t position,
	void *data)
{
	dn_list_t *list = position._internal._list;

	if (!list->head)
		position.it = list_insert_node_before (list->_internal._allocator, list->head, data);
	else if (!position.it)
		position.it = list_insert_node_after (list->_internal._allocator, list->tail, data);
	else
		position.it = list_insert_node_before (list->_internal._allocator, position.it, data);

	if (position.it) {
		if (!position.it->prev)
			list->head = position.it;
		if (!position.it->next)
			list->tail = position.it;
	}

	dn_list_result_t result = { position.it != NULL, { position.it, {position._internal._list } } };
	return result;
}

dn_list_result_t
dn_list_insert_range (
	dn_list_it_t position,
	dn_list_it_t first,
	dn_list_it_t last)
{
	dn_list_result_t first_inserted = { true, { position.it, {position._internal._list } } };

	if (first.it == last.it)
		return first_inserted;

	first_inserted = dn_list_insert (position, first.it->data);

	for (first.it = first.it->next; first.it && first.it != last.it; first.it = first.it->next)
		dn_list_insert (position, first.it->data);

	if (last.it)
		dn_list_insert (position, last.it->data);

	return first_inserted;
}

dn_list_it_t
dn_list_custom_erase (
	dn_list_it_t position,
	dn_func_t func)
{
	if (DN_UNLIKELY(!position.it))
		return position;

	dn_list_t *list = position._internal._list;

	if (position.it == list->head) {
		if (func)
			func (*dn_list_front (list));
		dn_list_pop_front (list);
		position.it = list->head;
	} else if (position.it == list->tail) {
		if (func)
			func (*dn_list_back (list));
		dn_list_pop_back (list);
		position.it = NULL;
	} else if (position.it) {
		dn_list_node_t *to_remove = position.it;
		position.it = position.it->next;
		list_dispose_node (list->_internal._allocator, func, list_unlink_node (to_remove));
	}

	return position;
}

bool
dn_list_push_back (
	dn_list_t *list,
	void *data)
{
	if (DN_UNLIKELY(!list))
		return false;

	return dn_list_insert (dn_list_end (list), data).result;
}

void
dn_list_pop_back (dn_list_t *list)
{
	if (DN_UNLIKELY(!list || !list->tail))
		return;

	dn_list_node_t *prev = list->tail->prev;
	list_free_node (list->_internal._allocator, list_unlink_node (list->tail));

	list->tail = prev;
	if (!list->tail)
		list->head = NULL;
}

bool
dn_list_push_front (
	dn_list_t *list,
	void *data)
{
	if (DN_UNLIKELY(!list))
		return false;

	return dn_list_insert (dn_list_begin (list), data).result;
}

void
dn_list_pop_front (dn_list_t *list)
{
	if (DN_UNLIKELY(!list || !list->head))
		return;

	dn_list_node_t *next = list->head->next;
	list_free_node (list->_internal._allocator, list_unlink_node (list->head));

	list->head = next;
	if (!list->head)
		list->tail = NULL;
}

bool
dn_list_custom_resize (
	dn_list_t *list,
	uint32_t count,
	dn_func_t func)
{
	if (DN_UNLIKELY(!list))
		return false;

	if (count == 0) {
		dn_list_custom_clear (list, func);
		return true;
	}

	dn_list_node_t *current = list->head;
	uint32_t i = 0;
	while (current) {
		i++;
		if (i == count) {
			dn_list_node_t *to_dispose = current->next;
			while (to_dispose) {
				dn_list_node_t *next = to_dispose->next;
				list_dispose_node (list->_internal._allocator, func, list_unlink_node (to_dispose));
				to_dispose = next;
			}
			list->tail = current;
			break;
		}
		current = current->next;
	}

	while (count++ < i)
		dn_list_insert (dn_list_end (list), NULL);

	return true;
}

void
dn_list_custom_remove (
	dn_list_t *list,
	const void *data,
	dn_func_t func)
{
	if (DN_UNLIKELY (!list))
		return;

	dn_list_node_t *current = list->head;
	dn_list_node_t *next;

	while (current) {
		next = current->next;
		if (current->data == data) {
			if (current == list->head)
				list->head = next;
			if (current == list->tail)
				list->tail = current->prev;
			list_dispose_node (list->_internal._allocator, func, list_unlink_node (current));
		}
		current = next;
	}
}

void
dn_list_custom_remove_if (
	dn_list_t *list,
	dn_predicate_func_t predicate,
	void * data,
	dn_func_t func)
{
	if (DN_UNLIKELY (!list))
		return;

	dn_list_node_t *current = list->head;
	dn_list_node_t *next;

	while (current) {
		next = current->next;
		if (!predicate || predicate (current->data, data)) {
			if (current == list->head)
				list->head = next;
			if (current == list->tail)
				list->tail = current->prev;
			list_dispose_node (list->_internal._allocator, func, list_unlink_node (current));
		}
		current = next;
	}
}

void
dn_list_reverse (dn_list_t *list)
{
	if (DN_UNLIKELY (!list))
		return;

	dn_list_node_t *node = list->head;
	dn_list_node_t *reverse;

	list->head = list->tail;
	list->tail = node;

	while (node) {
		reverse = node;
		node = reverse->next;

		reverse->next = reverse->prev;
		reverse->prev = node;
	}
}

void
dn_list_for_each (
	dn_list_t *list,
	dn_func_data_t foreach_func,
	void *data)
{
	if (DN_UNLIKELY (!list || !foreach_func))
		return;

	for (dn_list_node_t *it = list->head; it; it = it->next)
		foreach_func (it->data, data);
}

typedef dn_list_node_t list_node;
#include "dn-sort-frag.inc"

void
dn_list_sort (
	dn_list_t *list,
	dn_compare_func_t compare_func)
{
	if (DN_UNLIKELY (!list || !list->head || !list->head->next || !compare_func))
		return;

	list->head = do_sort (list->head, compare_func);
	list->head->prev = NULL;

	dn_list_node_t *current;
	for (current = list->head; current->next; current = current->next)
		current->next->prev = current;

	list->tail = current;
}

dn_list_it_t
dn_list_find (
	dn_list_t *list,
	const void *data,
	dn_compare_func_t compare_func)
{
	dn_list_it_t found;
	found.it = NULL;
	found._internal._list = list;

	if (DN_UNLIKELY (!list))
		return found;

	for (dn_list_node_t *it = list->head; it; it = it->next) {
		if ((compare_func && !compare_func (it->data, data)) || it->data == data) {
			found.it = it;
			break;
		}
	}

	return found;
}
