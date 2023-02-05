// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "dn-fwd-list.h"

dn_fwd_list_node_t _fwd_list_before_begin_it_node = { 0 };

static dn_fwd_list_node_t *
fwd_list_new_node (
	dn_allocator_t *allocator,
	dn_fwd_list_node_t *node,
	void *data)
{
	dn_fwd_list_node_t *new_node = (dn_fwd_list_node_t *)dn_allocator_alloc (allocator, sizeof (dn_fwd_list_node_t));
	if (DN_UNLIKELY (!new_node))
		return NULL;

	new_node->data = data;
	new_node->next = node;

	return new_node;
}

static dn_fwd_list_node_t *
fwd_list_insert_node_before (
	dn_allocator_t *allocator,
	dn_fwd_list_node_t *node,
	void *data)
{
	return fwd_list_new_node (allocator, node, data);
}

static dn_fwd_list_node_t *
fwd_list_insert_node_after (
	dn_allocator_t *allocator,
	dn_fwd_list_node_t *node,
	void *data)
{
	node->next = fwd_list_insert_node_before (allocator, node->next, data);
	return node->next;
}

static void
fwd_list_free_node (
	dn_allocator_t *allocator,
	dn_fwd_list_node_t *node)
{
	dn_allocator_free (allocator, node);
}

static void
fwd_list_dispose_node (
	dn_allocator_t *allocator,
	dn_func_t func,
	dn_fwd_list_node_t *node)
{
	if (node && func)
		func (node->data);
	dn_allocator_free (allocator, node);
}

dn_fwd_list_t *
_dn_fwd_list_alloc (dn_allocator_t *allocator)
{
	dn_fwd_list_t *list = (dn_fwd_list_t *)dn_allocator_alloc (allocator, sizeof (dn_fwd_list_t));
	if (!_dn_fwd_list_init (list, allocator)) {
		dn_allocator_free (allocator, list);
		return NULL;
	}

	return list;
}

bool
_dn_fwd_list_init (
	dn_fwd_list_t *list,
	dn_allocator_t *allocator)
{
	if (DN_UNLIKELY (!list))
		return false;

	memset (list, 0, sizeof(dn_fwd_list_t));
	list->_internal._allocator = allocator;

	return true;
}

void
dn_fwd_list_custom_free (
	dn_fwd_list_t *list,
	dn_func_t func)
{
	if (list) {
		dn_fwd_list_custom_dispose (list, func);
		dn_allocator_free (list->_internal._allocator, list);
	}
}

void
dn_fwd_list_custom_dispose (
	dn_fwd_list_t *list,
	dn_func_t func)
{
	if (DN_UNLIKELY(!list))
		return;

	dn_fwd_list_node_t *current = list->head;
	while (current) {
		dn_fwd_list_node_t *next = current->next;
		if (func)
			func (current->data);
		dn_allocator_free (list->_internal._allocator, current);
		current = next;
	}
}

void
dn_fwd_list_custom_clear (
	dn_fwd_list_t *list,
	dn_func_t func)
{
	if (DN_UNLIKELY(!list))
		return;

	dn_fwd_list_custom_dispose (list, func);

	list->head = NULL;
	list->tail = NULL;
}

dn_fwd_list_it_t
dn_fwd_list_insert_after (
	dn_fwd_list_it_t position,
	void *data,
	bool *result)
{
	dn_fwd_list_t *list = position._internal._list;

	if (position.it == &_fwd_list_before_begin_it_node || !list->head) {
		position.it = fwd_list_insert_node_before (list->_internal._allocator, list->head, data);
		list->head = position.it;
	} else if (!position.it) {
		position.it = fwd_list_insert_node_after (list->_internal._allocator, list->tail, data);
	} else {
		position.it = fwd_list_insert_node_after (list->_internal._allocator, position.it, data);
	}

	if (position.it && !position.it->next)
		list->tail = position.it;

	if (result)
		*result = position.it;

	return position;
}

dn_fwd_list_it_t
dn_fwd_list_insert_range_after (
	dn_fwd_list_it_t position,
	dn_fwd_list_it_t first,
	dn_fwd_list_it_t last,
	bool *result)
{
	if (first.it == last.it)
		return position;

	dn_fwd_list_it_t inserted = { NULL, { position._internal._list } };
	for (; first.it && first.it != last.it; first.it = first.it->next)
		inserted = dn_fwd_list_insert_after (position, first.it->data, result);

	if (last.it)
		inserted = dn_fwd_list_insert_after (position, last.it->data, result);

	return inserted;
}

dn_fwd_list_it_t
dn_fwd_list_custom_erase_after (
	dn_fwd_list_it_t position,
	dn_func_t func)
{
	dn_fwd_list_t *list = position._internal._list;

	if (position.it) {
		if (position.it == &_fwd_list_before_begin_it_node) {
			if (func)
				func (*dn_fwd_list_front (list));
			dn_fwd_list_pop_front (list);
			position.it = list->head;
		} else if (position.it->next) {
			dn_fwd_list_node_t *to_erase = position.it->next;
			position.it->next = position.it->next->next;
			fwd_list_dispose_node (position._internal._list->_internal._allocator, func, to_erase);
		}

		if (!position.it->next) {
			list->tail = position.it;
			position.it = NULL;
		}
	}

	return position;
}

bool
dn_fwd_list_push_front (
	dn_fwd_list_t *list,
	void *data)
{
	if (DN_UNLIKELY(!list))
		return false;

	bool result;
	dn_fwd_list_insert_after (dn_fwd_list_before_begin (list), data, &result);
	return result;
}

void
dn_fwd_list_pop_front (dn_fwd_list_t *list)
{
	if (DN_UNLIKELY(!list || !list->head))
		return;

	dn_fwd_list_node_t *next = list->head->next;
	fwd_list_free_node (list->_internal._allocator, list->head);

	list->head = next;
	if (!list->head)
		list->tail = NULL;
}

bool
dn_fwd_list_custom_resize (
	dn_fwd_list_t *list,
	uint32_t count,
	dn_func_t func)
{
	if (DN_UNLIKELY(!list))
		return false;

	if (count == 0) {
		dn_fwd_list_clear (list);
		return false;
	}

	dn_fwd_list_node_t *current = list->head;
	uint32_t i = 0;
	while (current) {
		i++;
		if (i == count) {
			dn_fwd_list_node_t *to_dispose = current->next;

			while (to_dispose) {
				dn_fwd_list_node_t *next = to_dispose->next;
				fwd_list_dispose_node (list->_internal._allocator, func, to_dispose);
				to_dispose = next;
			}

			current->next = NULL;
			list->tail = current;
			break;
		}
		current = current->next;
	}

	while (count++ < i)
		dn_fwd_list_insert_after (dn_fwd_list_end (list), NULL, NULL);

	return true;
}

void
dn_fwd_list_custom_remove (
	dn_fwd_list_t *list,
	const void *data,
	dn_func_t func)
{
	if (DN_UNLIKELY (!list))
		return;

	dn_fwd_list_node_t *current = list->head;
	dn_fwd_list_node_t *prev = current;
	dn_fwd_list_node_t *next;

	while (current) {
		next = current->next;
		if (current->data == data) {
			if (current == list->head) {
				list->head = next;
			} else if (current == list->tail) {
				prev->next = NULL;
				list->tail = prev;
			} else {
				prev->next = next;
			}
			fwd_list_dispose_node (list->_internal._allocator, func, current);
		} else {
			prev = current;
		}

		current = next;
	}
}

void
dn_fwd_list_custom_remove_if (
	dn_fwd_list_t *list,
	dn_predicate_func_t predicate,
	void *data,
	dn_func_t func)
{
	if (DN_UNLIKELY (!list))
		return;

	dn_fwd_list_node_t *current = list->head;
	dn_fwd_list_node_t *prev = current;
	dn_fwd_list_node_t *next;

	while (current) {
		next = current->next;
		if (!predicate || predicate (current->data, data)) {
			if (current == list->head) {
				list->head = next;
			} else if (current == list->tail) {
				prev->next = NULL;
				list->tail = prev;
			} else {
				prev->next = next;
			}
			fwd_list_dispose_node (list->_internal._allocator, func, current);
		} else {
			prev = current;
		}

		current = next;
	}
}

void
dn_fwd_list_reverse (dn_fwd_list_t *list)
{
	if (DN_UNLIKELY (!list))
		return;

	dn_fwd_list_node_t *node = list->head;
	dn_fwd_list_node_t *next;
	dn_fwd_list_node_t *prev = NULL;

	list->tail = list->head;

	while (node) {
		next = node->next;
		node->next = prev;

		prev = node;
		node = next;
	}

	list->head = prev;
}

void
dn_fwd_list_for_each (
	dn_fwd_list_t *list,
	dn_func_data_t foreach_func,
	void *data)
{
	if (DN_UNLIKELY (!list || !foreach_func))
		return;

	for (dn_fwd_list_node_t *it = list->head; it; it = it->next)
		foreach_func (it->data, data);
}

typedef dn_fwd_list_node_t list_node;
#include "dn-sort-frag.inc"

void
dn_fwd_list_sort (
	dn_fwd_list_t *list,
	dn_compare_func_t compare_func)
{
	if (DN_UNLIKELY (!list || !compare_func))
		return;
	
	list->head = do_sort (list->head, compare_func);

	dn_fwd_list_node_t *current = list->head;
	while (current->next)
		current = current->next;

	list->tail = current;

}

dn_fwd_list_it_t
dn_fwd_list_find (
	dn_fwd_list_t *list,
	const void *data,
	dn_compare_func_t compare_func)
{
	dn_fwd_list_it_t found;
	found.it = NULL;
	found._internal._list = list;

	if (DN_UNLIKELY (!list))
		return found;

	for (dn_fwd_list_node_t *it = list->head; it; it = it->next) {
		if ((compare_func && !compare_func (it->data, data)) || it->data == data) {
			found.it = it;
			break;
		}
	}

	return found;
}
