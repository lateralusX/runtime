/*
 * glist.c: Doubly-linked list implementation
 *
 * Authors:
 *   Duncan Mak (duncan@novell.com)
 *   Raja R Harinath (rharinath@novell.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (C) 2006 Novell, Inc.
 */
#include "dn-list-ex.h"
#include "dn-malloc.h"

dn_list_t *
dn_list_alloc (void)
{
	return dn_new0 (dn_list_t, 1);
}

static dn_list_t *
new_node (
	dn_list_t *prev,
	void *data,
	dn_list_t *next)
{
	dn_list_t *node = dn_list_alloc ();
	if (node) {
		node->data = data;
		node->prev = prev;
		node->next = next;
		if (prev)
			prev->next = node;
		if (next)
			next->prev = node;
	}
	return node;
}

static dn_list_t*
disconnect_node (dn_list_t *node)
{
	if (node->next)
		node->next->prev = node->prev;
	if (node->prev)
		node->prev->next = node->next;
	return node;
}

dn_list_t *
dn_list_prepend (
	dn_list_t *list,
	void *data)
{
	return new_node (list ? list->prev : NULL, data, list);
}

void
dn_list_free_1 (dn_list_t *list)
{
	dn_free (list);
}

void
dn_list_free (dn_list_t *list)
{
	while (list) {
		dn_list_t *next = list->next;
		dn_list_free_1 (list);
		list = next;
	}
}

dn_list_t *
dn_list_append (
	dn_list_t *list,
	void *data)
{
	dn_list_t *node = new_node (dn_list_last (list), data, NULL);
	return list ? list : node;
}

dn_list_t *
dn_list_concat (
	dn_list_t *list1,
	dn_list_t *list2)
{
	if (list1 && list2) {
		list2->prev = dn_list_last (list1);
		list2->prev->next = list2;
	}
	return list1 ? list1 : list2;
}

uint32_t
dn_list_length (dn_list_t *list)
{
	uint32_t length = 0;

	while (list) {
		length ++;
		list = list->next;
	}

	return length;
}

dn_list_t *
dn_list_remove (
	dn_list_t *list,
	const void *data)
{
	dn_list_t *current = dn_list_find (list, data);
	if (!current)
		return list;

	if (current == list)
		list = list->next;
	dn_list_free_1 (disconnect_node (current));

	return list;
}

dn_list_t *
dn_list_remove_all (
	dn_list_t *list,
	const void *data)
{
	dn_list_t *current = dn_list_find (list, data);
	if (!current)
		return list;

	while (current) {
		if (current == list)
			list = list->next;
		dn_list_free_1 (disconnect_node (current));

		current = dn_list_find (list, data);
	}

	return list;
}

dn_list_t *
dn_list_remove_link (
	dn_list_t *list,
	dn_list_t *link)
{
	if (list == link)
		list = list->next;

	disconnect_node (link);
	link->next = NULL;
	link->prev = NULL;

	return list;
}

dn_list_t *
dn_list_delete_link (
	dn_list_t *list,
	dn_list_t *link)
{
	list = dn_list_remove_link (list, link);
	dn_list_free_1 (link);

	return list;
}

dn_list_t *
dn_list_find (
	dn_list_t *list,
	const void *data)
{
	while (list){
		if (list->data == data)
			return list;

		list = list->next;
	}

	return NULL;
}

dn_list_t *
dn_list_find_custom (
	dn_list_t *list,
	const void *data,
	dn_compare_func_t func)
{
	if (!func)
		return NULL;

	while (list) {
		if (func (list->data, data) == 0)
			return list;

		list = list->next;
	}

	return NULL;
}

dn_list_t *
dn_list_reverse (dn_list_t *list)
{
	dn_list_t *reverse = NULL;

	while (list) {
		reverse = list;
		list = reverse->next;

		reverse->next = reverse->prev;
		reverse->prev = list;
	}

	return reverse;
}

dn_list_t *
dn_list_first (dn_list_t *list)
{
	if (!list)
		return NULL;

	while (list->prev)
		list = list->prev;

	return list;
}

dn_list_t *
dn_list_last (dn_list_t *list)
{
	if (!list)
		return NULL;

	while (list->next)
		list = list->next;

	return list;
}

dn_list_t *
dn_list_insert_sorted (
	dn_list_t *list,
	void *data,
	dn_compare_func_t func)
{
	dn_list_t *prev = NULL;
	dn_list_t *current;
	dn_list_t *node;

	if (!func)
		return list;

	/* Invariant: !prev || func (prev->data, data) <= 0) */
	for (current = list; current; current = current->next) {
		if (func (current->data, data) > 0)
			break;
		prev = current;
	}

	node = new_node (prev, data, current);
	return list == current ? node : list;
}

dn_list_t *
dn_list_insert_before (
	dn_list_t *list,
	dn_list_t *sibling,
	void *data)
{
	if (sibling) {
		dn_list_t *node = new_node (sibling->prev, data, sibling);
		return list == sibling ? node : list;
	}
	return dn_list_append (list, data);
}

void
dn_list_foreach (
	dn_list_t *list,
	dn_func_t func,
	void *user_data)
{
	while (list){
		(*func) (list->data, user_data);
		list = list->next;
	}
}

int32_t
dn_list_index (
	dn_list_t *list,
	const void *data)
{
	int32_t index = 0;

	while (list){
		if (list->data == data)
			return index;

		index ++;
		list = list->next;
	}

	return -1;
}

dn_list_t *
dn_list_nth (
	dn_list_t *list,
	uint32_t n)
{
	for (; list; list = list->next) {
		if (n == 0)
			break;
		n--;
	}
	return list;
}

void *
dn_list_nth_data (
	dn_list_t *list,
	uint32_t n)
{
	dn_list_t *node = dn_list_nth (list, n);
	return node ? node->data : NULL;
}

dn_list_t *
dn_list_copy (const dn_list_t *list)
{
	dn_list_t *copy = NULL;

	if (list) {
		dn_list_t *tmp = new_node (NULL, list->data, NULL);
		copy = tmp;

		for (list = list->next; list; list = list->next)
			tmp = new_node (tmp, list->data, NULL);
	}

	return copy;
}

void
dn_list_ex_free (dn_list_t **list)
{
	dn_list_free (*list);
	*list = NULL;
}

void
dn_list_ex_for_each_free (
	dn_list_t **list,
	dn_free_func_t func)
{
	if (*list && func) {
		for (dn_list_t *it = *list; it; it = it->next)
			func (it->data);
	}

	dn_list_free (*list);
	*list = NULL;
}

bool
dn_list_ex_push_back (
	dn_list_t **list,
	void *data)
{
	dn_list_t *result = dn_list_append (*list, data);
	if (result)
		*list = result;
	return result != NULL;
}

bool
dn_list_ex_push_front (
	dn_list_t **list,
	void *data)
{
	dn_list_t *result = dn_list_prepend (*list, data);
	if (result)
		*list = result;
	return result != NULL;
}

bool
dn_list_ex_erase (
	dn_list_t **list,
	const void *data)
{
	dn_list_t *current = dn_list_find (*list, data);
	if (!current)
		return false;

	if (current == *list)
		*list = (*list)->next;
	dn_list_free_1 (disconnect_node (current));

	return true;
}

bool
dn_list_ex_find(
	const dn_list_t *list,
	const void *data,
	dn_list_t **found)
{
	*found = dn_list_find ((dn_list_t *)list, data);
	return *found != NULL;
}

bool
dn_list_ex_find_custom(
	const dn_list_t *list,
	const void *data,
	dn_compare_func_t func,
	dn_list_t **found)
{
	*found = dn_list_find_custom ((dn_list_t *)list, data, func);
	return *found != NULL;
}

typedef dn_list_t list_node;
#include "dn-sort-frag.h"

dn_list_t *
dn_list_sort (
	dn_list_t *list,
	dn_compare_func_t func)
{
	dn_list_t *current;
	if (!list || !list->next)
		return list;
	list = do_sort (list, func);

	/* Fixup: do_sort doesn't update 'prev' pointers */
	list->prev = NULL;
	for (current = list; current->next; current = current->next)
		current->next->prev = current;

	return list;
}
