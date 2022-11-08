
#ifndef __DN_LIST_H__
#define __DN_LIST_H__

#include "dn-utils.h"

typedef struct _dn_list_t dn_list_t;

struct _dn_list_t {
	void *data;
	dn_list_t *next;
	dn_list_t *prev;
};

dn_list_t *
dn_list_alloc (void);

dn_list_t *
dn_list_append (
	dn_list_t *list,
	void *data);

dn_list_t *
dn_list_prepend (
	dn_list_t *list,
	void *data);

void
dn_list_free (dn_list_t *list);

void
dn_list_free_1 (dn_list_t *list);

dn_list_t *
dn_list_copy (const dn_list_t *list);

dn_list_t *
dn_list_concat (
	dn_list_t *list1,
	dn_list_t *list2);

void
dn_list_foreach (
	dn_list_t *list,
	dn_func_t func,
	void *user_data);

dn_list_t *
dn_list_last (dn_list_t *list);

dn_list_t *
dn_list_first (dn_list_t *list);

dn_list_t *
dn_list_find (
	dn_list_t *list,
	const void *data);

dn_list_t *
dn_list_find_custom (
	dn_list_t *list,
	const void *data,
	dn_compare_func_t func);

dn_list_t *
dn_list_remove (
	dn_list_t *list,
	const void *data);

dn_list_t *
dn_list_remove_all (
	dn_list_t *list,
	const void *data);

dn_list_t *
dn_list_reverse (dn_list_t *list);

uint32_t
dn_list_length (dn_list_t *list);

dn_list_t *
dn_list_remove_link (
	dn_list_t *list,
	dn_list_t *link);

dn_list_t *
dn_list_delete_link (
	dn_list_t *list,
	dn_list_t *link);

dn_list_t *
dn_list_insert_sorted (
	dn_list_t *list,
	void *data,
	dn_compare_func_t func);

dn_list_t *
dn_list_insert_before (
	dn_list_t *list,
	dn_list_t *sibling,
	void *data);

dn_list_t *
dn_list_sort (
	dn_list_t *list,
	dn_compare_func_t func);

int32_t
dn_list_index (
	dn_list_t *list,
	const void *data);

dn_list_t *
dn_list_nth (
	dn_list_t *list,
	uint32_t n);

void *
dn_list_nth_data (
	dn_list_t *list,
	uint32_t n);

#define dn_list_next(list) \
	((list) ? (((dn_list_t *) (list))->next) : NULL)

#define dn_list_previous(list) \
	((list) ? (((dn_list_t *) (list))->prev) : NULL)

#endif /* __DN_SLIST_H__ */
