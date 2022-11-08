
#ifndef __DN_LIST_EX_H__
#define __DN_LIST_EX_H__

#include "dn-utils.h"
#include "dn-list.h"


#define DN_LIST_EX_FOREACH_BEGIN(list,var_type,var_name) do { \
	var_type var_name; \
	for (dn_list_t *__it##var_name = dn_list_first (list); __it##var_name; __it##var_name = dn_list_next (__it##var_name)) { \
		var_name = (var_type)__it##var_name->data;

#define DN_LIST_EX_FOREACH_RBEGIN(list,var_type,var_name) do { \
	var_type var_name; \
	for (dn_list_t *__it##var_name = dn_list_last (list); __it##var_name; __it##var_name = dn_list_previous (__it##var_name)) { \
		var_name = (var_type)__it##var_name->data;

#define DN_LIST_EX_FOREACH_END \
		} \
	} while (0)

static inline dn_list_t *
dn_list_ex_alloc (void)
{
	return dn_list_alloc ();
}

void
dn_list_ex_free (dn_list_t **list);

void
dn_list_ex_for_each_free (
	dn_list_t **list,
	dn_free_func_t func);

static inline void
dn_list_ex_clear (dn_list_t **list)
{
	dn_list_ex_free (list);
}

static inline void
dn_list_ex_for_each_clear (
	dn_list_t **list,
	dn_free_func_t func)
{
	dn_list_ex_for_each_free (list, func);
}

bool
dn_list_ex_push_back (
	dn_list_t **list,
	void *data);

bool
dn_list_ex_push_front (
	dn_list_t **list,
	void *data);

bool
dn_list_ex_erase (
	dn_list_t **list,
	const void *data);

bool
dn_list_ex_find (
	const dn_list_t *list,
	const void *data,
	dn_list_t **found);

bool
dn_list_ex_find_custom (
	const dn_list_t *list,
	const void *data,
	dn_compare_func_t func,
	dn_list_t **found);

#define dn_list_ex_empty(list) \
	(list == NULL)

#define dn_list_ex_data(a,t) \
	(t)(a->data)

#endif /* __DN_LIST_EX_H__ */
