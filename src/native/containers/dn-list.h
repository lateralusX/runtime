
#ifndef __DN_LIST_H__
#define __DN_LIST_H__

#include "dn-utils.h"
#include "dn-allocator.h"

typedef struct _dn_list_node_t dn_list_node_t;
struct _dn_list_node_t {
	void *data;
	dn_list_node_t *next;
	dn_list_node_t *prev;
};

typedef struct _dn_list_t dn_list_t;
struct _dn_list_t {
	dn_list_node_t *head;
	dn_list_node_t *tail;
	struct {
		dn_allocator_t *_allocator;
	} _internal;
};

typedef struct _dn_list_it_t dn_list_it_t;
struct _dn_list_it_t {
	dn_list_node_t *it;
	struct {
		dn_list_t *_list;
	} _internal;
};

typedef struct _dn_list_result_t dn_list_result_t;
struct _dn_list_result_t {
	bool result;
	dn_list_it_t it;
};

static inline void
dn_list_it_advance (
	dn_list_it_t *it,
	ptrdiff_t n)
{
	while (n > 0 && it->it) {
		it->it = it->it->next;
		n--;
	}

	while (n < 0 && it->it) {
		it->it = it->it->prev;
		n++;
	}
}

static inline dn_list_it_t
dn_list_it_next (dn_list_it_t it)
{
	it.it = it.it->next;
	return it;
}

static inline dn_list_it_t
dn_list_it_prev (dn_list_it_t it)
{
	it.it = it.it->prev;
	return it;
}

static inline dn_list_it_t
dn_list_it_next_n (
	dn_list_it_t it,
	uint32_t n)
{
	dn_list_it_advance (&it, n);
	return it;
}

static inline dn_list_it_t
dn_list_it_prev_n (
	dn_list_it_t it,
	uint32_t n)
{
	dn_list_it_advance (&it, -(ptrdiff_t)n);
	return it;
}

static inline void **
dn_list_it_data (dn_list_it_t it)
{
	return &(it.it->data);
}

#define dn_list_it_data_t(it, type) \
	(type *)dn_list_it_data ((it))

static inline bool
dn_list_it_begin (dn_list_it_t it)
{
	return !(it.it == it._internal._list->head);
}

static inline bool
dn_list_it_end (dn_list_it_t it)
{
	return !(it.it);
}

#define DN_LIST_FOREACH_BEGIN(list,var_type,var_name) do { \
	var_type var_name; \
	for (dn_list_node_t *__it##var_name = (list)->head; __it##var_name; __it##var_name = __it##var_name->next) { \
		var_name = (var_type)__it##var_name->data;

#define DN_LIST_FOREACH_RBEGIN(list,var_type,var_name) do { \
	var_type var_name; \
	for (dn_list_node_t *__it##var_name = (list)->tail; __it##var_name; __it##var_name = __it##var_name->prev) { \
		var_name = (var_type)__it##var_name->data;

#define DN_LIST_FOREACH_END \
		} \
	} while (0)

dn_list_t *
_dn_list_alloc (dn_allocator_t *allocator);

bool
_dn_list_init (
	dn_list_t *list,
	dn_allocator_t *allocator);

static inline dn_list_t *
dn_list_custom_alloc (
	dn_allocator_t *allocator)
{
	return _dn_list_alloc (allocator);
}

static inline dn_list_t *
dn_list_alloc (void)
{
	return _dn_list_alloc (DN_DEFAULT_ALLOCATOR);
}

void
dn_list_custom_free (
	dn_list_t *list,
	dn_func_t func);

static inline void
dn_list_free (dn_list_t *list)
{
	dn_list_custom_free (list, NULL);
}

static inline bool
dn_list_custom_init (
	dn_list_t *list,
	dn_allocator_t *allocator)
{
	return _dn_list_init (list, allocator);
}

static inline bool
dn_list_init (dn_list_t *list)
{
	return _dn_list_init (list, DN_DEFAULT_ALLOCATOR);
}

void
dn_list_custom_dispose (
	dn_list_t *list,
	dn_func_t func);

static inline void
dn_list_dispose (dn_list_t *list)
{
	dn_list_custom_dispose (list, NULL);
}

static inline void **
dn_list_front (const dn_list_t *list)
{
	if (DN_UNLIKELY (!list || !list->head))
		return NULL;

	return &(list->head->data);
}

#define dn_list_front_t(list, type) \
	(type *)dn_list_front ((list))

static inline void **
dn_list_back (const dn_list_t *list)
{
	if (DN_UNLIKELY (!list || !list->tail))
		return NULL;

	return &(list->tail->data);
}

#define dn_list_back_t(list, type) \
	(type *)dn_list_back ((list))

static inline dn_list_it_t
dn_list_begin (dn_list_t *list)
{
	dn_list_it_t it = { list->head, { list } };
	return it;
}

static inline dn_list_it_t
dn_list_end (dn_list_t *list)
{
	dn_list_it_t it = { NULL, { list } };
	return it;
}

static inline bool
dn_list_empty (const dn_list_t *list)
{
	return !list || list->head == NULL;
}

uint32_t
dn_list_size (const dn_list_t *list);

static inline uint32_t
dn_list_max_size (const dn_list_t *list)
{
	DN_UNREFERENCED_PARAMETER (list);
	return UINT32_MAX;
}

void
dn_list_custom_clear (
	dn_list_t *list,
	dn_func_t func);


static inline void
dn_list_clear (dn_list_t *list)
{
	dn_list_custom_clear (list, NULL);
}

dn_list_result_t
dn_list_insert (
	dn_list_it_t position,
	void *data);

dn_list_result_t
dn_list_insert_range (
	dn_list_it_t position,
	dn_list_it_t first,
	dn_list_it_t list);

dn_list_it_t
dn_list_custom_erase (
	dn_list_it_t position,
	dn_func_t func);

static inline dn_list_it_t
dn_list_erase (dn_list_it_t position)
{
	return dn_list_custom_erase (position, NULL);
}

bool
dn_list_push_back (
	dn_list_t *list,
	void *data);

void
dn_list_pop_back (dn_list_t *list);

bool
dn_list_push_front (
	dn_list_t *list,
	void *data);

void
dn_list_pop_front (dn_list_t *list);

bool
dn_list_custom_resize (
	dn_list_t *list,
	uint32_t count,
	dn_func_t func);

static inline bool
dn_list_resize (
	dn_list_t *list,
	uint32_t count)
{
	return dn_list_custom_resize (list, count, NULL);
}

void
dn_list_custom_remove (
	dn_list_t *list,
	const void *data,
	dn_func_t func);

static inline void
dn_list_remove (
	dn_list_t *list,
	const void *data)
{
	dn_list_custom_remove (list, data, NULL);
}

void
dn_list_custom_remove_if (
	dn_list_t *list,
	dn_predicate_func_t predicate,
	void *data,
	dn_func_t func);

static inline void
dn_list_remove_if (
	dn_list_t *list,
	dn_predicate_func_t predicate,
	void *data)
{
	dn_list_custom_remove_if (list, predicate, data, NULL);
}

void
dn_list_reverse (dn_list_t *list);

void
dn_list_for_each (
	dn_list_t *list,
	dn_func_data_t foreach_func,
	void *data);

void
dn_list_sort (
	dn_list_t *list,
	dn_compare_func_t compare_func);

dn_list_it_t
dn_list_find (
	dn_list_t *list,
	const void *data,
	dn_compare_func_t compare_func);

#endif /* __DN_SLIST_H__ */
