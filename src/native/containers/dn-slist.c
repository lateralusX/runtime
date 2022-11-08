/*
 * gslist.c: Singly-linked list implementation
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
#include "dn-slist-ex.h"
#include "dn-malloc.h"

dn_slist_t *
dn_slist_alloc (void)
{
	return dn_new0 (dn_slist_t, 1);
}

void
dn_slist_free_1 (dn_slist_t *list)
{
	dn_free (list);
}

dn_slist_t *
dn_slist_append (
	dn_slist_t *list,
	void *data)
{
	return dn_slist_concat (list, dn_slist_prepend (NULL, data));
}

dn_slist_t *
dn_slist_prepend (
	dn_slist_t *list,
	void *data)
{
	dn_slist_t *head = dn_slist_alloc ();
	if (DN_UNLIKELY(!head))
		return NULL;

	head->data = data;
	head->next = list;

	return head;
}

static dn_slist_t *
insert_after (
	dn_slist_t *list,
	void *data)
{
	list->next = dn_slist_prepend (list->next, data);
	return list->next;
}

static dn_slist_t *
find_prev (
	dn_slist_t *list,
	const void *data)
{
	dn_slist_t *prev = NULL;
	while (list) {
		if (list->data == data)
			break;
		prev = list;
		list = list->next;
	}
	return prev;
}

static dn_slist_t *
find_prev_link (
	dn_slist_t *list,
	const dn_slist_t *link)
{
	dn_slist_t *prev = NULL;
	while (list) {
		if (list == link)
			break;
		prev = list;
		list = list->next;
	}
	return prev;
}

dn_slist_t *
dn_slist_insert_before (
	dn_slist_t *list,
	dn_slist_t *sibling,
	void *data)
{
	dn_slist_t *prev = find_prev_link (list, sibling);

	if (!prev)
		return dn_slist_prepend (list, data);

	insert_after (prev, data);
	return list;
}

void
dn_slist_free (dn_slist_t *list)
{
	while (list) {
		dn_slist_t *next = list->next;
		dn_slist_free_1 (list);
		list = next;
	}
}

dn_slist_t *
dn_slist_copy (const dn_slist_t *list)
{
	dn_slist_t *copy, *tmp;

	if (!list)
		return NULL;

	copy = dn_slist_prepend (NULL, list->data);
	tmp = copy;

	for (list = list->next; list; list = list->next)
		tmp = insert_after (tmp, list->data);

	return copy;
}

dn_slist_t *
dn_slist_concat (
	dn_slist_t *list1,
	dn_slist_t *list2)
{
	if (!list1)
		return list2;

	dn_slist_last (list1)->next = list2;
	return list1;
}

void
dn_slist_foreach (
	dn_slist_t *list,
	dn_func_t func,
	void *user_data)
{
	while (list) {
		(*func) (list->data, user_data);
		list = list->next;
	}
}

dn_slist_t *
dn_slist_last (dn_slist_t *list)
{
	if (!list)
		return NULL;

	while (list->next)
		list = list->next;

	return list;
}

dn_slist_t *
dn_slist_find (
	dn_slist_t *list,
	const void *data)
{
	for (; list; list = list->next)
		if (list->data == data)
			break;
	return list;
}

dn_slist_t *
dn_slist_find_custom (
	dn_slist_t *list,
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

uint32_t
dn_slist_length (dn_slist_t *list)
{
	uint32_t length = 0;

	while (list) {
		length ++;
		list = list->next;
	}

	return length;
}

dn_slist_t *
dn_slist_remove (
	dn_slist_t *list,
	const void *data)
{
	dn_slist_t *prev = find_prev (list, data);
	dn_slist_t *current = prev ? prev->next : list;

	if (current) {
		if (prev)
			prev->next = current->next;
		else
			list = current->next;
		dn_slist_free_1 (current);
	}

	return list;
}

dn_slist_t *
dn_slist_remove_all (
	dn_slist_t *list,
	const void *data)
{
	dn_slist_t *next = list;
	dn_slist_t *prev = NULL;
	dn_slist_t *current;

	while (next) {
		dn_slist_t *tmp_prev = find_prev (next, data);
		if (tmp_prev)
			prev = tmp_prev;
		current = prev ? prev->next : list;

		if (!current)
			break;

		next = current->next;

		if (prev)
			prev->next = next;
		else
			list = next;
		dn_slist_free_1 (current);
	}

	return list;
}

dn_slist_t *
dn_slist_remove_link (
	dn_slist_t *list,
	dn_slist_t *link)
{
	dn_slist_t *prev = find_prev_link (list, link);
	dn_slist_t *current = prev ? prev->next : list;

	if (current) {
		if (prev)
			prev->next = current->next;
		else
			list = current->next;
		current->next = NULL;
	}

	return list;
}

dn_slist_t *
dn_slist_delete_link (
	dn_slist_t *list,
	dn_slist_t *link)
{
	list = dn_slist_remove_link (list, link);
	dn_slist_free_1 (link);

	return list;
}

dn_slist_t *
dn_slist_reverse (dn_slist_t *list)
{
	dn_slist_t *prev = NULL;
	while (list){
		dn_slist_t *next = list->next;
		list->next = prev;
		prev = list;
		list = next;
	}

	return prev;
}

dn_slist_t *
dn_slist_insert_sorted (
	dn_slist_t *list,
	void *data,
	dn_compare_func_t func)
{
	dn_slist_t *prev = NULL;

	if (!func)
		return list;

	if (!list || func (list->data, data) > 0)
		return dn_slist_prepend (list, data);

	for (prev = list; prev->next; prev = prev->next)
		if (func (prev->next->data, data) > 0)
			break;

	insert_after (prev, data);
	return list;
}

int32_t
dn_slist_index (
	dn_slist_t *list,
	const void *data)
{
	int32_t index = 0;

	while (list) {
		if (list->data == data)
			return index;

		index++;
		list = list->next;
	}

	return -1;
}

dn_slist_t *
dn_slist_nth (
	dn_slist_t *list,
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
dn_slist_nth_data (
	dn_slist_t *list,
	uint32_t n)
{
	dn_slist_t *node = dn_slist_nth (list, n);
	return node ? node->data : NULL;
}

void
dn_slist_ex_free (dn_slist_t **list)
{
	dn_slist_free (*list);
	*list = NULL;
}

void
dn_slist_ex_for_each_free (
	dn_slist_t **list,
	dn_free_func_t func)
{
	if (*list && func) {
		for (dn_slist_t *it = *list; it; it = it->next)
			func (it->data);
	}
	
	dn_slist_free (*list);
	*list = NULL;
}

bool
dn_slist_ex_push_back (
	dn_slist_t **list,
	void *data)
{
	dn_slist_t *result = dn_slist_append (*list, data);
	if (result)
		*list = result;
	return result != NULL;
}

bool
dn_slist_ex_push_front (
	dn_slist_t **list,
	void *data)
{
	dn_slist_t *result = dn_slist_prepend (*list, data);
	if (result)
		*list = result;
	return result != NULL;
}

bool
dn_slist_ex_erase (
	dn_slist_t **list,
	const void *data)
{
	bool result = false;
	dn_slist_t *prev = find_prev (*list, data);
	dn_slist_t *current = prev ? prev->next : *list;

	if (current) {
		if (prev)
			prev->next = current->next;
		else
			*list = current->next;

		dn_slist_free_1 (current);
		result = true;
	}

	return result;
}

bool
dn_slist_ex_find(
	const dn_slist_t *list,
	const void *data,
	dn_slist_t **found)
{
	*found = dn_slist_find ((dn_slist_t *)list, data);
	return *found != NULL;
}

bool
dn_slist_ex_find_custom(
	const dn_slist_t *list,
	const void *data,
	dn_compare_func_t func,
	dn_slist_t **found)
{
	*found = dn_slist_find_custom ((dn_slist_t *)list, data, func);
	return *found != NULL;
}

typedef dn_slist_t list_node;
#include "dn-sort-frag.h"

dn_slist_t *
dn_slist_sort (
	dn_slist_t *list,
	dn_compare_func_t func)
{
	if (!list || !list->next)
		return list;
	return do_sort (list, func);
}
