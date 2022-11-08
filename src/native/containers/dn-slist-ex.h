
#ifndef __DN_SLIST_EX_H__
#define __DN_SLIST_EX_H__

#include "dn-utils.h"
#include "dn-slist.h"

#define DN_SLIST_EX_FOREACH_BEGIN(list,var_type,var_name) do { \
	var_type var_name; \
	for (dn_slist_t *__it##var_name = list; __it##var_name; __it##var_name = dn_slist_next (__it##var_name)) { \
		var_name = (var_type)__it##var_name->data;

#define DN_SLIST_EX_FOREACH_END \
		} \
	} while (0)

static inline dn_slist_t *
dn_slist_ex_alloc (void)
{
	return dn_slist_alloc ();
}

void
dn_slist_ex_free (dn_slist_t **list);

void
dn_slist_ex_for_each_free (
	dn_slist_t **list,
	dn_free_func_t func);

static inline void
dn_slist_ex_clear (dn_slist_t **list)
{
	dn_slist_ex_free (list);
}

static inline void
dn_slist_ex_for_each_clear (
	dn_slist_t **list,
	dn_free_func_t func)
{
	dn_slist_ex_for_each_free (list, func);
}

bool
dn_slist_ex_push_back (
	dn_slist_t **list,
	void *data);

bool
dn_slist_ex_push_front (
	dn_slist_t **list,
	void *data);

bool
dn_slist_ex_erase (
	dn_slist_t **list,
	const void *data);

bool
dn_slist_ex_find (
	const dn_slist_t *list,
	const void *data,
	dn_slist_t **found);

bool
dn_slist_ex_find_custom (
	const dn_slist_t *list,
	const void *data,
	dn_compare_func_t func,
	dn_slist_t **found);

#define dn_slist_ex_empty(list) \
	(list == NULL)

#define dn_slist_ex_data(a,t) \
	(t)(a->data)

#endif /* __DN_SLIST_EX_H__ */
