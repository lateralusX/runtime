/*
 *
 * Author:
 *   Duncan Mak (duncan@novell.com)
 *   Gonzalo Paniagua Javier (gonzalo@novell.com)
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
 * Copyright (c) 2006-2009 Novell, Inc.
 *
 */
#include "dn-queue-ex.h"
#include "dn-malloc.h"

void *
dn_queue_pop_head (dn_queue_t *queue)
{
	void *result;
	dn_list_t *old_head;

	if (!queue || !queue->head || queue->length == 0)
		return NULL;

	result = queue->head->data;
	old_head = queue->head;
	queue->head = old_head->next;
	dn_list_free_1 (old_head);

	if (--queue->length)
		queue->head->prev = NULL;
	else
		queue->tail = NULL;

	return result;
}

bool
dn_queue_is_empty (dn_queue_t *queue)
{
	if (!queue)
		return true;

	return queue->length == 0;
}

void
dn_queue_push_head (
	dn_queue_t *queue,
	void *data)
{
	if (!queue || queue->length == UINT32_MAX)
		return;

	dn_list_t *head = dn_list_prepend (queue->head, data);
	if (!head)
		return;

	queue->head = head;

	if (!queue->tail)
		queue->tail = queue->head;

	queue->length++;
}

void
dn_queue_push_tail (
	dn_queue_t *queue,
	void *data)
{
	if (!queue || queue->length == UINT32_MAX)
		return;

	dn_list_t *tail = dn_list_append (queue->tail, data);
	if (!tail)
		return;

	queue->tail = tail;
	if (queue->head == NULL)
		queue->head = queue->tail;
	else
		queue->tail = queue->tail->next;
	queue->length++;
}

dn_queue_t *
dn_queue_new (void)
{
	return dn_new0 (dn_queue_t, 1);
}

void
dn_queue_free (dn_queue_t *queue)
{
	if (!queue)
		return;

	dn_list_free (queue->head);
	dn_free (queue);
}

void
dn_queue_foreach (
	dn_queue_t *queue,
	dn_func_t func,
	void *user_data)
{
	if (!queue)
		return;

	dn_list_foreach (queue->head, func, user_data);
}

dn_queue_t *
dn_queue_ex_alloc (void)
{
	return dn_queue_new ();
}

void
dn_queue_ex_free (dn_queue_t **queue)
{
	dn_queue_free (*queue);
	*queue = NULL;
}

void
dn_queue_ex_for_each_free (
	dn_queue_t **queue,
	dn_free_func_t func)
{
	if (*queue && func)
		dn_list_ex_for_each_free (&(*queue)->head, func);
	
	dn_free (*queue);
	*queue = NULL;
}

void
dn_queue_ex_clear(dn_queue_t *queue)
{
	if (!queue)
		return;

	dn_list_ex_free (&queue->head);
	memset (queue, 0, sizeof (dn_queue_t));
}

void
dn_queue_ex_for_each_clear (
	dn_queue_t *queue,
	dn_free_func_t func)
{
	if (!queue)
		return;

	dn_list_ex_for_each_clear (&queue->head, func);
	memset (queue, 0, sizeof (dn_queue_t));
}
