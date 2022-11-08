
#ifndef __DN_SLIST_H__
#define __DN_SLIST_H__

#include "dn-utils.h"

typedef struct _dn_slist_t dn_slist_t;

struct _dn_slist_t {
	void *data;
	dn_slist_t *next;
};

dn_slist_t *
dn_slist_alloc (void);

dn_slist_t *
dn_slist_append (
	dn_slist_t *list,
	void *data);

dn_slist_t *
dn_slist_prepend (
	dn_slist_t *list,
	void *data);

void
dn_slist_free (dn_slist_t *list);

void
dn_slist_free_1 (dn_slist_t *list);

dn_slist_t *
dn_slist_copy (const dn_slist_t *list);

dn_slist_t *
dn_slist_concat (
	dn_slist_t *list1,
	dn_slist_t *list2);

void
dn_slist_foreach (
	dn_slist_t *list,
	dn_func_t func,
	void *user_data);

dn_slist_t *
dn_slist_last (dn_slist_t *list);

dn_slist_t *
dn_slist_find (
	dn_slist_t *list,
	const void *data);

dn_slist_t *
dn_slist_find_custom (
	dn_slist_t *list,
	const void *data,
	dn_compare_func_t func);

dn_slist_t *
dn_slist_remove (
	dn_slist_t *list,
	const void *data);

dn_slist_t *
dn_slist_remove_all (
	dn_slist_t *list,
	const void *data);

dn_slist_t *
dn_slist_reverse (dn_slist_t *list);

uint32_t
dn_slist_length (dn_slist_t *list);

dn_slist_t *
dn_slist_remove_link (
	dn_slist_t *list,
	dn_slist_t *link);

dn_slist_t *
dn_slist_delete_link (
	dn_slist_t *list,
	dn_slist_t *link);

dn_slist_t *
dn_slist_insert_sorted (
	dn_slist_t *list,
	void *data,
	dn_compare_func_t func);

dn_slist_t *
dn_slist_insert_before (
	dn_slist_t *list,
	dn_slist_t *sibling,
	void *data);

dn_slist_t *
dn_slist_sort (
	dn_slist_t *list,
	dn_compare_func_t func);

int32_t
dn_slist_index (
	dn_slist_t *list,
	const void *data);

dn_slist_t *
dn_slist_nth (
	dn_slist_t *list,
	uint32_t n);

void *
dn_slist_nth_data (
	dn_slist_t *list,
	uint32_t n);

#define dn_slist_next(slist) \
	((slist) ? (((dn_slist_t *) (slist))->next) : NULL)

#endif /* __DN_SLIST_H__ */
