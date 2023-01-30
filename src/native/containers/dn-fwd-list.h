
#ifndef __DN_FWD_LIST_H__
#define __DN_FWD_LIST_H__

#include "dn-utils.h"
#include "dn-allocator.h"

typedef struct _dn_fwd_list_node_t dn_fwd_list_node_t;
struct _dn_fwd_list_node_t {
	void *data;
	dn_fwd_list_node_t *next;
};

typedef struct _dn_fwd_list_t dn_fwd_list_t;
struct _dn_fwd_list_t {
	dn_fwd_list_node_t *head;
	dn_fwd_list_node_t *tail;
	struct {
		dn_allocator_t *_allocator;
		dn_dispose_func_t _dispose_func;
	} _internal;
};

typedef struct _dn_fwd_list_it_t dn_fwd_list_it_t;
struct _dn_fwd_list_it_t {
	dn_fwd_list_node_t *it;
	struct {
		dn_fwd_list_t *_list;
	} _internal;
};

static inline void
dn_fwd_list_it_advance (
	dn_fwd_list_it_t *it,
	uint32_t n)
{
	while (n > 0 && it->it) {
		it->it = it->it->next;
		n--;
	}
}

static inline dn_fwd_list_it_t
dn_fwd_list_it_next (dn_fwd_list_it_t it)
{
	it.it = it.it->next;
	return it;
}

static inline dn_fwd_list_it_t
dn_fwd_list_it_next_n (
	dn_fwd_list_it_t it,
	uint32_t n)
{
	dn_fwd_list_it_advance (&it, n);
	return it;
}

static inline void **
dn_fwd_list_it_data (dn_fwd_list_it_t *it)
{
	return &(it->it->data);
}

#define dn_fwd_list_it_data_t(it, type) \
	(type *)dn_fwd_list_it_data ((it))

static inline bool
dn_fwd_list_it_begin (dn_fwd_list_it_t it)
{
	return !(it.it == it._internal._list->head);
}

static inline bool
dn_fwd_list_it_end (dn_fwd_list_it_t it)
{
	return !(it.it);
}

#define DN_FWD_LIST_FOREACH_BEGIN(list,var_type,var_name) do { \
	var_type var_name; \
	for (dn_fwd_list_node_t *__it##var_name = (list)->head; __it##var_name; __it##var_name = __it##var_name->next) { \
		var_name = (var_type)__it##var_name->data;

#define DN_FWD_LIST_FOREACH_END \
		} \
	} while (0)

dn_fwd_list_t *
_dn_fwd_list_alloc (
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func);

bool
_dn_fwd_list_init (
	dn_fwd_list_t *list,
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func);

static inline dn_fwd_list_t *
dn_fwd_list_custom_alloc (
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func)
{
	return _dn_fwd_list_alloc (allocator, dispose_func);
}

static inline dn_fwd_list_t *
dn_fwd_list_alloc (void)
{
	return _dn_fwd_list_alloc (DN_DEFAULT_ALLOCATOR, NULL);
}

void
dn_fwd_list_free (dn_fwd_list_t *list);

static inline bool
dn_fwd_list_custom_init (
	dn_fwd_list_t *list,
	dn_allocator_t *allocator,
	dn_dispose_func_t dispose_func)
{
	return _dn_fwd_list_init (list, allocator, dispose_func);
}

static inline bool
dn_fwd_list_init (dn_fwd_list_t *list)
{
	return _dn_fwd_list_init (list, DN_DEFAULT_ALLOCATOR, NULL);
}

void
dn_fwd_list_dispose (dn_fwd_list_t *list);

static inline void **
dn_fwd_list_front (const dn_fwd_list_t *list)
{
	if (DN_UNLIKELY (!list || !list->head))
		return NULL;
	
	return &(list->head->data);
}

#define dn_fwd_list_front_t(list, type) \
	(type *)dn_fwd_list_front ((list))

static inline dn_fwd_list_it_t
dn_fwd_list_before_begin (dn_fwd_list_t *list)
{
	extern dn_fwd_list_node_t _fwd_list_before_begin_it_node;

	dn_fwd_list_it_t it;
	it._internal._list = list;
	it.it = &_fwd_list_before_begin_it_node;
	return it;
}

static inline dn_fwd_list_it_t
dn_fwd_list_begin (dn_fwd_list_t *list)
{
	dn_fwd_list_it_t it;
	it._internal._list = list;
	it.it = list->head;
	return it;
}

static inline dn_fwd_list_it_t
dn_fwd_list_end (dn_fwd_list_t *list)
{
	dn_fwd_list_it_t it;
	it._internal._list = list;
	it.it = NULL;
	return it;
}

static inline bool
dn_fwd_list_empty (const dn_fwd_list_t *list)
{
	return !list || list->head == NULL;
}

static inline uint32_t
dn_fwd_list_max_size (const dn_fwd_list_t *list)
{
	DN_UNREFERENCED_PARAMETER (list);
	return UINT32_MAX;
}

void
dn_fwd_list_clear (dn_fwd_list_t *list);

dn_fwd_list_it_t
dn_fwd_list_insert_after (
	dn_fwd_list_it_t position,
	void *data,
	bool *result);

dn_fwd_list_it_t
dn_fwd_list_insert_range_after (
	dn_fwd_list_it_t position,
	dn_fwd_list_it_t first,
	dn_fwd_list_it_t last,
	bool *result);

dn_fwd_list_it_t
dn_fwd_list_erase_after (dn_fwd_list_it_t position);

bool
dn_fwd_list_push_front (
	dn_fwd_list_t *list,
	void *data);

void
dn_fwd_list_pop_front (dn_fwd_list_t *list);

void
dn_fwd_list_resize (
	dn_fwd_list_t *list,
	uint32_t count);

void
dn_fwd_list_remove (
	dn_fwd_list_t *list,
	const void *data);

void
dn_fwd_list_remove_if (
	dn_fwd_list_t *list,
	dn_remove_func_t remove_func,
	void *user_data);

void
dn_fwd_list_reverse (dn_fwd_list_t *list);

void
dn_fwd_list_for_each (
	dn_fwd_list_t *list,
	dn_func_t foreach_func,
	void *user_data);

void
dn_fwd_list_sort (
	dn_fwd_list_t *list,
	dn_compare_func_t compare_func);

dn_fwd_list_it_t
dn_fwd_list_find (
	dn_fwd_list_t *list,
	const void *data,
	dn_compare_func_t compare_func);

#endif /* __DN_FWD_LIST_H__ */
