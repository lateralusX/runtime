/*
 *
 * Author:
 *   Miguel de Icaza (miguel@novell.com)
 *
 * (C) 2006 Novell, Inc.
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
 */
#include "dn-unordered-map.h"
#include "dn-malloc.h"
#include <minipal/utils.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

static void * KEYMARKER_REMOVED = &KEYMARKER_REMOVED;

static const uint32_t prime_tbl[] = {
	11, 19, 37, 73, 109, 163, 251, 367, 557, 823, 1237,
	1861, 2777, 4177, 6247, 9371, 14057, 21089, 31627,
	47431, 71143, 106721, 160073, 240101, 360163,
	540217, 810343, 1215497, 1823231, 2734867, 4102283,
	6153409, 9230113, 13845163
};

static bool
test_prime (int32_t x)
{
	if ((x & 1) != 0) {
		int32_t n;
		for (n = 3; n < (int32_t)sqrt (x); n += 2) {
			if ((x % n) == 0)
				return false;
		}
		return true;
	}
	// There is only one even prime - 2.
	return (x == 2);
}

static int32_t
calc_prime (int32_t x)
{
	int32_t i;

	for (i = (x & (~1))-1; i < INT32_MAX; i += 2) {
		if (test_prime (i))
			return i;
	}
	return x;
}

static uint32_t
spaced_primes_closest (uint32_t x)
{
	for (size_t i = 0; i < ARRAY_SIZE (prime_tbl); i++) {
		if (x <= prime_tbl [i])
			return prime_tbl [i];
	}
	return calc_prime (x);
}

dn_unordered_map_t *
dn_unordered_map_new (
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func)
{
	dn_unordered_map_t *map;

	if (hash_func == NULL)
		hash_func = dn_direct_hash;
	if (equal_func == NULL)
		equal_func = dn_direct_equal;
	map = dn_new0 (dn_unordered_map_t, 1);

	map->hash_func = hash_func;
	map->key_equal_func = equal_func;

	map->table_size = spaced_primes_closest (1);
	map->table = dn_new0 (dn_unordered_map_slot_t *, map->table_size);
	map->last_rehash = map->table_size;

	return map;
}

dn_unordered_map_t *
dn_unordered_map_new_full (
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_destory_notify_func_t key_destroy_func,
	dn_destory_notify_func_t value_destroy_func)
{
	dn_unordered_map_t *map = dn_unordered_map_new (hash_func, equal_func);
	if (map == NULL)
		return NULL;

	map->key_destroy_func = key_destroy_func;
	map->value_destroy_func = value_destroy_func;

	return map;
}

#ifdef SANITY_CHECK
static void
dump (dn_unordered_map_t *map)
{
	for (int32_t i = 0; i < map->table_size; i++) {
		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; s = s->next){
			uint32_t hashcode = (*map->hash_func) (s->key);
			uint32_t slot = (hashcode) % map->table_size;
			printf ("key %p hash %x on slot %d correct slot %d tb size %d\n", s->key, hashcode, i, slot, map->table_size);
		}
	}
}

static void
sanity_check (dn_unordered_map_t *map)
{
	for (int32_t i = 0; i < map->table_size; i++) {
		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; s = s->next){
			uint32_t hashcode = (*map->hash_func) (s->key);
			uint32_t slot = (hashcode) % map->table_size;
			if (slot != i) {
				hashcode = (*map->hash_func) (s->key);
				g_error ("Key %p (bucket %d) on invalid bucket %d (hashcode %x) (tb size %d)", s->key, slot, i, hashcode, map->table_size);
			}
		}
	}
}
#else

#define dump(map) do {}while(0)
#define sanity_check(map) do {}while(0)

#endif

static void
do_rehash (dn_unordered_map_t *map)
{
	int32_t current_size;
	dn_unordered_map_slot_t **table;

	map->last_rehash = map->table_size;
	current_size = map->table_size;
	map->table_size = spaced_primes_closest (map->in_use);
	table = map->table;
	map->table = dn_new0 (dn_unordered_map_slot_t *, map->table_size);

	for (int32_t i = 0; i < current_size; i++){
		dn_unordered_map_slot_t *s, *next;

		for (s = table [i]; s != NULL; s = next){
			uint32_t hashcode = ((*map->hash_func) (s->key)) % map->table_size;
			next = s->next;

			s->next = map->table [hashcode];
			map->table [hashcode] = s;
		}
	}
	dn_free (table);
}

static void
rehash (dn_unordered_map_t *map)
{
	int32_t diff = abs (map->last_rehash - map->in_use);

	if (!(diff * 0.75 > map->table_size * 2))
		return;

	do_rehash (map);
	sanity_check (map);
}

void
dn_unordered_map_insert_replace (
	dn_unordered_map_t *map,
	void *key,
	void *value,
	bool replace)
{
	uint32_t hashcode;
	dn_equal_func_t equal;

	if (!map)
		return;

	sanity_check (map);

	equal = map->key_equal_func;
	if (map->in_use >= map->threshold)
		rehash (map);

	hashcode = ((*map->hash_func) (key)) % map->table_size;
	for (dn_unordered_map_slot_t *s = map->table [hashcode]; s != NULL; s = s->next){
		if ((*equal) (s->key, key)){
			if (replace){
				if (map->key_destroy_func != NULL)
					(*map->key_destroy_func)(s->key);
				s->key = key;
			}
			if (map->value_destroy_func != NULL)
				(*map->value_destroy_func) (s->value);
			s->value = value;
			sanity_check (map);
			return;
		}
	}

	dn_unordered_map_slot_t *s = dn_new (dn_unordered_map_slot_t, 1);
	s->key = key;
	s->value = value;
	s->next = map->table [hashcode];
	map->table [hashcode] = s;
	map->in_use++;
	sanity_check (hash);
}

uint32_t
dn_unordered_map_size (dn_unordered_map_t *map)
{
	if (!map)
		return 0;

	return map->in_use;
}

dn_list_t *
dn_unordered_map_get_keys (dn_unordered_map_t *map)
{
	dn_unordered_map_iter_t iter;
	dn_list_t *rv = NULL;
	void *key;

	dn_unordered_map_iter_init (&iter, map);

	while (dn_unordered_map_iter_next (&iter, &key, NULL))
		rv = dn_list_prepend (rv, key);

	return dn_list_reverse (rv);
}

dn_list_t *
dn_unordered_map_get_values (dn_unordered_map_t *map)
{
	dn_unordered_map_iter_t iter;
	dn_list_t *rv = NULL;
	void *value;

	dn_unordered_map_iter_init (&iter, map);

	while (dn_unordered_map_iter_next (&iter, NULL, &value))
		rv = dn_list_prepend (rv, value);

	return dn_list_reverse (rv);
}

bool
dn_unordered_map_contains (
	dn_unordered_map_t *map,
	const void *key)
{
	if (DN_UNLIKELY (!map))
		return false;

	return dn_unordered_map_lookup_extended (map, key, NULL, NULL);
}

void *
dn_unordered_map_lookup (
	dn_unordered_map_t *map,
	const void *key)
{
	void *orig_key, *value;

	if (dn_unordered_map_lookup_extended (map, key, &orig_key, &value))
		return value;
	else
		return NULL;
}

bool
dn_unordered_map_lookup_extended (
	dn_unordered_map_t *map,
	const void *key,
	void **orig_key,
	void **value)
{
	dn_equal_func_t equal;
	dn_unordered_map_slot_t *s;
	uint32_t hashcode;

	if (DN_UNLIKELY (!map))
		return false;

	sanity_check (map);
	equal = map->key_equal_func;

	hashcode = ((*map->hash_func) (key)) % map->table_size;

	for (s = map->table [hashcode]; s != NULL; s = s->next){
		if ((*equal)(s->key, key)){
			if (orig_key)
				*orig_key = s->key;
			if (value)
				*value = s->value;
			return true;
		}
	}
	return false;
}

void
dn_unordered_map_foreach (
	dn_unordered_map_t *map,
	dn_h_func_t func,
	void *user_data)
{
	if (DN_UNLIKELY (!map || !func))
		return;

	for (int32_t i = 0; i < map->table_size; i++){
		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; s = s->next)
			(*func)(s->key, s->value, user_data);
	}
}

void *
dn_unordered_map_find (
	dn_unordered_map_t *map,
	dn_hr_func_t predicate,
	void *user_data)
{
	if (DN_UNLIKELY (!map || !predicate))
		return NULL;

	for (int32_t i = 0; i < map->table_size; i++) {
		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; s = s->next)
			if ((*predicate)(s->key, s->value, user_data))
				return s->value;
	}
	return NULL;
}

bool
dn_unordered_map_remove (
	dn_unordered_map_t *map,
	const void *key)
{
	dn_equal_func_t equal;
	dn_unordered_map_slot_t *s, *last;
	uint32_t hashcode;

	if (DN_UNLIKELY (!map))
		return false;

	sanity_check (map);
	equal = map->key_equal_func;

	hashcode = ((*map->hash_func)(key)) % map->table_size;
	last = NULL;
	for (s = map->table [hashcode]; s != NULL; s = s->next){
		if ((*equal)(s->key, key)){
			if (map->key_destroy_func != NULL)
				(*map->key_destroy_func)(s->key);
			if (map->value_destroy_func != NULL)
				(*map->value_destroy_func)(s->value);
			if (last == NULL)
				map->table [hashcode] = s->next;
			else
				last->next = s->next;
			dn_free (s);
			map->in_use--;
			sanity_check (hash);
			return true;
		}
		last = s;
	}
	sanity_check (hash);
	return false;
}

bool
dn_unordered_map_steal (
	dn_unordered_map_t *map,
	const void *key)
{
	dn_equal_func_t equal;
	dn_unordered_map_slot_t *s, *last;
	uint32_t hashcode;

	if (DN_UNLIKELY (!map))
		return false;

	sanity_check (map);
	equal = map->key_equal_func;

	hashcode = ((*map->hash_func)(key)) % map->table_size;
	last = NULL;
	for (s = map->table [hashcode]; s != NULL; s = s->next){
		if ((*equal)(s->key, key)) {
			if (last == NULL)
				map->table [hashcode] = s->next;
			else
				last->next = s->next;
			dn_free (s);
			map->in_use--;
			sanity_check (map);
			return true;
		}
		last = s;
	}
	sanity_check (map);
	return false;
}

void
dn_unordered_map_remove_all (dn_unordered_map_t *map)
{
	if (DN_UNLIKELY (!map))
		return;

	for (int32_t i = 0; i < map->table_size; i++) {
		dn_unordered_map_slot_t *s;

		while (map->table [i]) {
			s = map->table [i];
			dn_unordered_map_remove (map, s->key);
		}
	}
}

uint32_t
dn_unordered_map_foreach_remove (
	dn_unordered_map_t *map,
	dn_hr_func_t func,
	void *user_data)
{
	int count = 0;

	if (DN_UNLIKELY (!map || !func))
		return 0;

	sanity_check (map);
	for (int32_t i = 0; i < map->table_size; i++) {
		dn_unordered_map_slot_t * last = NULL;
		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; ) {
			if ((*func)(s->key, s->value, user_data)) {
				dn_unordered_map_slot_t *n;
				if (map->key_destroy_func != NULL)
					(*map->key_destroy_func)(s->key);
				if (map->value_destroy_func != NULL)
					(*map->value_destroy_func)(s->value);
				if (last == NULL){
					map->table [i] = s->next;
					n = s->next;
				} else  {
					last->next = s->next;
					n = last->next;
				}
				dn_free (s);
				map->in_use--;
				count++;
				s = n;
			} else {
				last = s;
				s = s->next;
			}
		}
	}
	sanity_check (map);
	if (count > 0)
		rehash (map);
	return count;
}

uint32_t
dn_unordered_map_foreach_steal (
	dn_unordered_map_t *map,
	dn_hr_func_t func,
	void *user_data)
{
	int32_t count = 0;

	if (DN_UNLIKELY (!map || !func))
		return 0;

	sanity_check (map);
	for (int32_t i = 0; i < map->table_size; i++) {
		dn_unordered_map_slot_t *last = NULL;
		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; ) {
			if ((*func)(s->key, s->value, user_data)){
				dn_unordered_map_slot_t *n;

				if (last == NULL){
					map->table [i] = s->next;
					n = s->next;
				} else  {
					last->next = s->next;
					n = last->next;
				}
				dn_free (s);
				map->in_use--;
				count++;
				s = n;
			} else {
				last = s;
				s = s->next;
			}
		}
	}
	sanity_check (map);
	if (count > 0)
		rehash (map);
	return count;
}

void
dn_unordered_map_destroy (dn_unordered_map_t *map)
{
	if (DN_UNLIKELY(!map))
		return;

	for (int32_t i = 0; i < map->table_size; i++){
		dn_unordered_map_slot_t *s, *next;
		for (s = map->table [i]; s != NULL; s = next){
			next = s->next;

			if (map->key_destroy_func != NULL)
				(*map->key_destroy_func)(s->key);
			if (map->value_destroy_func != NULL)
				(*map->value_destroy_func)(s->value);
			dn_free (s);
		}
	}
	dn_free (map->table);

	dn_free (map);
}

void
dn_unordered_map_print_stats (dn_unordered_map_t *map)
{
	int32_t max_chain_index, chain_size, max_chain_size;

	max_chain_size = 0;
	max_chain_index = -1;
	for (int32_t i = 0; i < map->table_size; i++) {
		chain_size = 0;
		for (dn_unordered_map_slot_t *node = map->table [i]; node; node = node->next)
			chain_size ++;
		if (chain_size > max_chain_size) {
			max_chain_size = chain_size;
			max_chain_index = i;
		}
	}

	printf ("Size: %d Table Size: %d Max Chain Length: %d at %d\n", map->in_use, map->table_size, max_chain_size, max_chain_index);
}

void
dn_unordered_map_iter_init (
	dn_unordered_map_iter_t *iter,
	dn_unordered_map_t *map)
{
	memset (iter, 0, sizeof (dn_unordered_map_iter_t));
	iter->map = map;
	iter->index = -1;
}

bool
dn_unordered_map_iter_next (
	dn_unordered_map_iter_t *iter,
	void **key,
	void **value)
{
	if (DN_UNLIKELY(iter->index == -2))
		return false;

	if (!iter->slot) {
		while (true) {
			iter->index ++;
			if (iter->index >= iter->map->table_size) {
				iter->index = -2;
				return false;
			}
			if (iter->map->table [iter->index])
				break;
		}
		iter->slot = iter->map->table [iter->index];
	}

	if (key)
		*key = iter->slot->key;
	if (value)
		*value = iter->slot->value;
	iter->slot = iter->slot->next;

	return true;
}

bool
dn_direct_equal (const void *v1, const void *v2)
{
	return v1 == v2;
}

uint32_t
dn_direct_hash (const void *v1)
{
	return ((uint32_t)(size_t)(v1));
}

bool
dn_int_equal (const void *v1, const void *v2)
{
	return *(int32_t *)v1 == *(int32_t *)v2;
}

uint32_t
dn_int_hash (const void *v1)
{
	return *(uint32_t *)v1;
}

bool
dn_str_equal (const void *v1, const void *v2)
{
	return v1 == v2 || strcmp ((const char*)v1, (const char*)v2) == 0;
}

uint32_t
dn_str_hash (const void *v1)
{
	uint32_t hash = 0;
	char *p = (char *) v1;

	while (*p++)
		hash = (hash << 5) - (hash + *p);

	return hash;
}
