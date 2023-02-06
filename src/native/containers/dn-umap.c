// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

/* (C) 2006 Novell, Inc.
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
#include "dn-umap.h"
#include <minipal/utils.h>

#include <string.h>
#include <math.h>
//#include <stdio.h>

static void * KEYMARKER_REMOVED = &KEYMARKER_REMOVED;

static const uint32_t prime_tbl[] = {
	11, 19, 37, 73, 109, 163, 251, 367, 557, 823, 1237,
	1861, 2777, 4177, 6247, 9371, 14057, 21089, 31627,
	47431, 71143, 106721, 160073, 240101, 360163,
	540217, 810343, 1215497, 1823231, 2734867, 4102283,
	6153409, 9230113, 13845163
};

static bool
test_prime (uint32_t x)
{
	if ((x & 1) != 0) {
		uint32_t n;
		for (n = 3; n < (uint32_t)sqrt (x); n += 2) {
			if ((x % n) == 0)
				return false;
		}
		return true;
	}
	return (x == 2);
}

static uint32_t
calc_prime (uint32_t x)
{
	uint32_t i;

	for (i = (x & (~1))-1; i < UINT32_MAX; i += 2) {
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

#ifdef SANITY_CHECK
static void
dump (dn_umap_t *map)
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
sanity_check (dn_umap_t *map)
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
do_rehash (
	dn_umap_t *map,
	uint32_t new_bucket_count)
{
	uint32_t current_bucket_count;
	dn_umap_node_t **buckets;

	map->_internal._last_rehash = map->_internal._bucket_count;
	current_bucket_count = map->_internal._bucket_count;
	map->_internal._bucket_count = new_bucket_count;
	buckets = map->_internal._buckets;

	map->_internal._buckets = dn_allocator_alloc (map->_internal._allocator, sizeof (dn_umap_node_t *) * map->_internal._bucket_count);
	if (map->_internal._buckets)
		memset (map->_internal._buckets, 0, sizeof (dn_umap_node_t *) * map->_internal._bucket_count);

	for (uint32_t i = 0; i < current_bucket_count; i++){
		dn_umap_node_t *s, *next;

		for (s = buckets [i]; s != NULL; s = next){
			uint32_t hashcode = map->_internal._hash_func (s->key) % map->_internal._bucket_count;
			next = s->next;

			s->next = map->_internal._buckets [hashcode];
			map->_internal._buckets [hashcode] = s;
		}
	}

	dn_allocator_free (map->_internal._allocator, buckets);
}

static void
rehash (dn_umap_t *map)
{
	uint32_t diff;
	if (map->_internal._last_rehash > map->_internal._node_count)
		diff = map->_internal._last_rehash - map->_internal._node_count;
	else
		diff = map->_internal._node_count - map->_internal._last_rehash;

	if (!(diff * 0.75 > map->_internal._bucket_count * 2))
		return;

	do_rehash (map, spaced_primes_closest (map->_internal._node_count));
	sanity_check (map);
}

static dn_umap_result_t
umap_insert (
	dn_umap_t *map,
	void *key,
	void *value,
	bool replace_existing)
{
	uint32_t hashcode;
	dn_equal_func_t equal;

	dn_umap_result_t result = { false, { map, NULL, 0 } };

	if (!map)
		return result;

	sanity_check (map);

	equal = map->_internal._key_equal_func;
	if (map->_internal._node_count >= map->_internal._threshold)
		rehash (map);

	hashcode = map->_internal._hash_func (key) % map->_internal._bucket_count;
	for (dn_umap_node_t *s = map->_internal._buckets [hashcode]; s != NULL; s = s->next) {
		if (equal (s->key, key)) {
			if (!replace_existing) {
				result.it._internal._node = s;
				result.it._internal._index = hashcode;
				return result;
			}
			if (map->_internal._key_dispose_func)
				map->_internal._key_dispose_func (s->key);
			s->key = key;
			if (map->_internal._value_dispose_func)
				map->_internal._value_dispose_func (s->value);
			s->value = value;
			sanity_check (map);

			result.it._internal._index = hashcode;
			result.it._internal._node = s;
			result.result = true;

			return result;
		}
	}

	dn_umap_node_t *s = dn_allocator_alloc (map->_internal._allocator, sizeof (dn_umap_node_t));
	s->key = key;
	s->value = value;
	s->next = map->_internal._buckets [hashcode];
	map->_internal._buckets [hashcode] = s;
	map->_internal._node_count ++;
	sanity_check (hash);

	result.it._internal._index = hashcode;
	result.it._internal._node = s;
	result.result = true;

	return result;
}

dn_umap_it_t
_dn_umap_begin (dn_umap_t *map)
{
	dn_umap_it_t it = dn_umap_end (map);
	if (!DN_UNLIKELY(map))
		return it;

	uint32_t index = 0;
	while (true) {
		if (index >= it._internal._map->_internal._bucket_count) {
			return it;
		}
		if (it._internal._map->_internal._buckets [index])
			break;
		index ++;
	}
	it._internal._node = it._internal._map->_internal._buckets [index];
	it._internal._index = index;

	return it;
}

bool
_dn_umap_it_next (dn_umap_it_t *it)
{
	if (DN_UNLIKELY(dn_umap_it_end (*it)))
		return false;

	if (!it->_internal._node->next) {
		while (true) {
			it->_internal._index ++;
			if (it->_internal._index >= it->_internal._map->_internal._bucket_count) {
				*it = dn_umap_end (it->_internal._map);
				return false;
			}
			if (it->_internal._map->_internal._buckets [it->_internal._index])
				break;
		}
		it->_internal._node = it->_internal._map->_internal._buckets [it->_internal._index];
	} else {
		it->_internal._node = it->_internal._node->next;
	}

	return it->_internal._node;
}

dn_umap_t *
_dn_umap_alloc (
	dn_allocator_t *allocator,
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_func_t key_dispose_func,
	dn_func_t value_dispose_func)
{
	dn_umap_t *map = dn_allocator_alloc (allocator, sizeof (dn_umap_t));
	if (!_dn_umap_init (map, allocator, hash_func, equal_func, key_dispose_func, value_dispose_func)) {
		dn_allocator_free (allocator, map);
		return NULL;
	}

	return map;
}

bool
_dn_umap_init (
	dn_umap_t *map,
	dn_allocator_t *allocator,
	dn_hash_func_t hash_func,
	dn_equal_func_t equal_func,
	dn_func_t key_dispose_func,
	dn_func_t value_dispose_func)
{
	if (DN_UNLIKELY (!map))
		return false;

	memset (map, 0, sizeof(dn_umap_t));

	if (hash_func == NULL)
		hash_func = dn_direct_hash;
	if (equal_func == NULL)
		equal_func = dn_direct_equal;

	map->_internal._allocator = allocator;
	
	map->_internal._hash_func = hash_func;
	map->_internal._key_equal_func = equal_func;

	map->_internal._bucket_count = spaced_primes_closest (1);
	
	map->_internal._last_rehash = map->_internal._bucket_count;

	map->_internal._key_dispose_func = key_dispose_func;
	map->_internal._value_dispose_func = value_dispose_func;

	map->_internal._buckets = dn_allocator_alloc (allocator, sizeof (dn_umap_node_t *) * map->_internal._bucket_count);
	if (map->_internal._buckets)
		memset (map->_internal._buckets, 0, sizeof (dn_umap_node_t *) * map->_internal._bucket_count);

	return map->_internal._buckets;
}

void
dn_umap_free (dn_umap_t *map)
{
	if (map) {
		dn_umap_dispose (map);
		dn_allocator_free (map->_internal._allocator, map);
	}
}

void
dn_umap_dispose (dn_umap_t *map)
{
	if (DN_UNLIKELY(!map))
		return;

	for (uint32_t i = 0; i < map->_internal._bucket_count; i++){
		dn_umap_node_t *s, *next;
		for (s = map->_internal._buckets [i]; s != NULL; s = next){
			next = s->next;

			if (map->_internal._key_dispose_func)
				map->_internal._key_dispose_func (s->key);
			if (map->_internal._value_dispose_func)
				map->_internal._value_dispose_func (s->value);
			dn_allocator_free (map->_internal._allocator, s);
		}
	}
	dn_allocator_free (map->_internal._allocator, map->_internal._buckets);
}

void
dn_umap_clear (dn_umap_t *map)
{
	if (DN_UNLIKELY (!map))
		return;

	for (uint32_t i = 0; i < map->_internal._bucket_count; i++) {
		dn_umap_node_t *s;

		while (map->_internal._buckets [i]) {
			s = map->_internal._buckets [i];
			dn_umap_erase_key (map, s->key);
		}
	}
}

dn_umap_result_t
dn_umap_insert (
	dn_umap_t *map,
	void *key,
	void *value)
{
	return umap_insert (map, key, value, false);
}

dn_umap_result_t
dn_umap_insert_or_assign (
	dn_umap_t *map,
	void *key,
	void *value)
{
	return umap_insert (map, key, value, true);
}

dn_umap_it_t
dn_umap_erase (dn_umap_it_t position)
{
	if (dn_umap_it_end (position))
		return position;

	dn_umap_it_t result = dn_umap_it_next (position);
	dn_umap_erase_key (position._internal._map, position._internal._node->key);
	return result;
}

uint32_t
dn_umap_erase_key (
	dn_umap_t *map,
	const void *key)
{
	dn_equal_func_t equal;
	dn_umap_node_t *s, *last;
	uint32_t hashcode;

	if (DN_UNLIKELY (!map))
		return 0;

	sanity_check (map);
	equal = map->_internal._key_equal_func;

	hashcode = map->_internal._hash_func (key) % map->_internal._bucket_count;
	last = NULL;
	for (s = map->_internal._buckets [hashcode]; s != NULL; s = s->next){
		if ((*equal)(s->key, key)){
			if (map->_internal._key_dispose_func)
				map->_internal._key_dispose_func (s->key);
			if (map->_internal._value_dispose_func)
				map->_internal._value_dispose_func (s->value);
			if (last == NULL)
				map->_internal._buckets [hashcode] = s->next;
			else
				last->next = s->next;
			dn_free (s);
			map->_internal._node_count --;
			sanity_check (hash);
			return 1;
		}
		last = s;
	}
	sanity_check (hash);
	return 0;
}

bool
dn_umap_extract_key (
	dn_umap_t *map,
	const void *key,
	void **out_key,
	void **out_value)
{
	dn_equal_func_t equal;
	dn_umap_node_t *s, *last;
	uint32_t hashcode;

	if (DN_UNLIKELY (!map))
		return false;

	sanity_check (map);
	equal = map->_internal._key_equal_func;

	hashcode = map->_internal._hash_func (key) % map->_internal._bucket_count;
	last = NULL;
	for (s = map->_internal._buckets [hashcode]; s != NULL; s = s->next){
		if ((*equal)(s->key, key)) {
			if (last == NULL)
				map->_internal._buckets [hashcode] = s->next;
			else
				last->next = s->next;

			if (out_key)
				*out_key = s->key;
			if (out_value)
				*out_value = s->value;

			dn_free (s);
			map->_internal._node_count --;
			sanity_check (map);

			return true;
		}
		last = s;
	}
	sanity_check (map);
	return false;
}

dn_umap_it_t
dn_umap_custom_find (
	dn_umap_t *map,
	const void *data,
	dn_equal_func_t func)
{
	dn_umap_it_t found = dn_umap_end (map);

	if (DN_UNLIKELY (!map))
		return found;

	for (uint32_t i = 0; i < map->_internal._bucket_count; i++) {
		for (dn_umap_node_t *s = map->_internal._buckets [i]; s != NULL; s = s->next)
			if (func && func (s->key, data)) {
				found._internal._index = i;
				found._internal._node = s;
				break;
			}
			else if (s->key == data) {
				found._internal._index = i;
				found._internal._node = s;
				break;
			}

	}

	return found;
}

void
dn_umap_for_each (
	dn_umap_t *map,
	dn_key_value_func_t func,
	void *data)
{
	if (DN_UNLIKELY (!map || !func))
		return;

	DN_UMAP_FOREACH_BEGIN (map, void *, key, void *, value) {
		func (key, value, data);
	} DN_UMAP_FOREACH_END;
}

void
dn_umap_rehash (
	dn_umap_t *map,
	uint32_t count)
{
	if (DN_UNLIKELY (!map))
		return;

	// count < (map->_internal._node_count / dn_umap_max_load_factor ())
		// count = map->_internal._node_count / dn_umap_max_load_factor ()
	if (count < map->_internal._node_count)
		count = map->_internal._node_count;

	do_rehash (map, count);
}

void
dn_umap_reserve (
	dn_umap_t *map,
	uint32_t count)
{
	if (DN_UNLIKELY (!map))
		return;

	// do_rehash(ceil(count / dn_umap_max_load_factor ()))
	do_rehash (map, count);
}

//void
//dn_unordered_map_insert_replace (
//	dn_unordered_map_t *map,
//	void *key,
//	void *value,
//	bool replace)
//{
//	uint32_t hashcode;
//	dn_equal_func_t equal;
//
//	if (!map)
//		return;
//
//	sanity_check (map);
//
//	equal = map->key_equal_func;
//	if (map->in_use >= map->threshold)
//		rehash (map);
//
//	hashcode = ((*map->hash_func) (key)) % map->table_size;
//	for (dn_unordered_map_slot_t *s = map->table [hashcode]; s != NULL; s = s->next){
//		if ((*equal) (s->key, key)){
//			if (replace){
//				if (map->key_destroy_func != NULL)
//					(*map->key_destroy_func)(s->key);
//				s->key = key;
//			}
//			if (map->value_destroy_func != NULL)
//				(*map->value_destroy_func) (s->value);
//			s->value = value;
//			sanity_check (map);
//			return;
//		}
//	}
//
//	dn_unordered_map_slot_t *s = dn_new (dn_unordered_map_slot_t, 1);
//	s->key = key;
//	s->value = value;
//	s->next = map->table [hashcode];
//	map->table [hashcode] = s;
//	map->in_use++;
//	sanity_check (hash);
//}
//
//uint32_t
//dn_unordered_map_size (dn_unordered_map_t *map)
//{
//	if (!map)
//		return 0;
//
//	return map->in_use;
//}
//
//dn_list_t *
//dn_unordered_map_get_keys (dn_unordered_map_t *map)
//{
//	dn_list_t *rv = dn_list_alloc ();
//	if (rv) {
//		void *key;
//		dn_unordered_map_iter_t iter;
//
//		dn_unordered_map_iter_init (&iter, map);
//		while (dn_unordered_map_iter_next (&iter, &key, NULL))
//			dn_list_push_front (rv, key);
//
//		dn_list_reverse (rv);
//	}
//
//	return rv;
//}
//
//dn_list_t *
//dn_unordered_map_get_values (dn_unordered_map_t *map)
//{
//	dn_list_t *rv = dn_list_alloc ();
//	if (rv) {
//		void *value;
//		dn_unordered_map_iter_t iter;
//
//		dn_unordered_map_iter_init (&iter, map);
//
//		while (dn_unordered_map_iter_next (&iter, NULL, &value))
//			dn_list_push_front (rv, value);
//
//		dn_list_reverse (rv);
//	}
//
//	return rv;
//}
//
//bool
//dn_unordered_map_contains (
//	dn_unordered_map_t *map,
//	const void *key)
//{
//	if (DN_UNLIKELY (!map))
//		return false;
//
//	return dn_unordered_map_lookup_extended (map, key, NULL, NULL);
//}
//
//void *
//dn_unordered_map_lookup (
//	dn_unordered_map_t *map,
//	const void *key)
//{
//	void *orig_key, *value;
//
//	if (dn_unordered_map_lookup_extended (map, key, &orig_key, &value))
//		return value;
//	else
//		return NULL;
//}
//
//bool
//dn_unordered_map_lookup_extended (
//	dn_unordered_map_t *map,
//	const void *key,
//	void **orig_key,
//	void **value)
//{
//	dn_equal_func_t equal;
//	dn_unordered_map_slot_t *s;
//	uint32_t hashcode;
//
//	if (DN_UNLIKELY (!map))
//		return false;
//
//	sanity_check (map);
//	equal = map->key_equal_func;
//
//	hashcode = ((*map->hash_func) (key)) % map->table_size;
//
//	for (s = map->table [hashcode]; s != NULL; s = s->next){
//		if ((*equal)(s->key, key)){
//			if (orig_key)
//				*orig_key = s->key;
//			if (value)
//				*value = s->value;
//			return true;
//		}
//	}
//	return false;
//}
//
//void
//dn_unordered_map_foreach (
//	dn_unordered_map_t *map,
//	dn_h_func_t func,
//	void *user_data)
//{
//	if (DN_UNLIKELY (!map || !func))
//		return;
//
//	for (int32_t i = 0; i < map->table_size; i++){
//		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; s = s->next)
//			(*func)(s->key, s->value, user_data);
//	}
//}
//
//void *
//dn_unordered_map_find (
//	dn_unordered_map_t *map,
//	dn_hr_func_t predicate,
//	void *user_data)
//{
//	if (DN_UNLIKELY (!map || !predicate))
//		return NULL;
//
//	for (int32_t i = 0; i < map->table_size; i++) {
//		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; s = s->next)
//			if ((*predicate)(s->key, s->value, user_data))
//				return s->value;
//	}
//	return NULL;
//}
//
//bool
//dn_unordered_map_remove (
//	dn_unordered_map_t *map,
//	const void *key)
//{
//	dn_equal_func_t equal;
//	dn_unordered_map_slot_t *s, *last;
//	uint32_t hashcode;
//
//	if (DN_UNLIKELY (!map))
//		return false;
//
//	sanity_check (map);
//	equal = map->key_equal_func;
//
//	hashcode = ((*map->hash_func)(key)) % map->table_size;
//	last = NULL;
//	for (s = map->table [hashcode]; s != NULL; s = s->next){
//		if ((*equal)(s->key, key)){
//			if (map->key_destroy_func != NULL)
//				(*map->key_destroy_func)(s->key);
//			if (map->value_destroy_func != NULL)
//				(*map->value_destroy_func)(s->value);
//			if (last == NULL)
//				map->table [hashcode] = s->next;
//			else
//				last->next = s->next;
//			dn_free (s);
//			map->in_use--;
//			sanity_check (hash);
//			return true;
//		}
//		last = s;
//	}
//	sanity_check (hash);
//	return false;
//}
//
//bool
//dn_unordered_map_steal (
//	dn_unordered_map_t *map,
//	const void *key)
//{
//	dn_equal_func_t equal;
//	dn_unordered_map_slot_t *s, *last;
//	uint32_t hashcode;
//
//	if (DN_UNLIKELY (!map))
//		return false;
//
//	sanity_check (map);
//	equal = map->key_equal_func;
//
//	hashcode = ((*map->hash_func)(key)) % map->table_size;
//	last = NULL;
//	for (s = map->table [hashcode]; s != NULL; s = s->next){
//		if ((*equal)(s->key, key)) {
//			if (last == NULL)
//				map->table [hashcode] = s->next;
//			else
//				last->next = s->next;
//			dn_free (s);
//			map->in_use--;
//			sanity_check (map);
//			return true;
//		}
//		last = s;
//	}
//	sanity_check (map);
//	return false;
//}
//
//void
//dn_unordered_map_remove_all (dn_unordered_map_t *map)
//{
//	if (DN_UNLIKELY (!map))
//		return;
//
//	for (int32_t i = 0; i < map->table_size; i++) {
//		dn_unordered_map_slot_t *s;
//
//		while (map->table [i]) {
//			s = map->table [i];
//			dn_unordered_map_remove (map, s->key);
//		}
//	}
//}
//
//uint32_t
//dn_unordered_map_foreach_remove (
//	dn_unordered_map_t *map,
//	dn_hr_func_t func,
//	void *user_data)
//{
//	int count = 0;
//
//	if (DN_UNLIKELY (!map || !func))
//		return 0;
//
//	sanity_check (map);
//	for (int32_t i = 0; i < map->table_size; i++) {
//		dn_unordered_map_slot_t * last = NULL;
//		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; ) {
//			if ((*func)(s->key, s->value, user_data)) {
//				dn_unordered_map_slot_t *n;
//				if (map->key_destroy_func != NULL)
//					(*map->key_destroy_func)(s->key);
//				if (map->value_destroy_func != NULL)
//					(*map->value_destroy_func)(s->value);
//				if (last == NULL){
//					map->table [i] = s->next;
//					n = s->next;
//				} else  {
//					last->next = s->next;
//					n = last->next;
//				}
//				dn_free (s);
//				map->in_use--;
//				count++;
//				s = n;
//			} else {
//				last = s;
//				s = s->next;
//			}
//		}
//	}
//	sanity_check (map);
//	if (count > 0)
//		rehash (map);
//	return count;
//}
//
//uint32_t
//dn_unordered_map_foreach_steal (
//	dn_unordered_map_t *map,
//	dn_hr_func_t func,
//	void *user_data)
//{
//	int32_t count = 0;
//
//	if (DN_UNLIKELY (!map || !func))
//		return 0;
//
//	sanity_check (map);
//	for (int32_t i = 0; i < map->table_size; i++) {
//		dn_unordered_map_slot_t *last = NULL;
//		for (dn_unordered_map_slot_t *s = map->table [i]; s != NULL; ) {
//			if ((*func)(s->key, s->value, user_data)){
//				dn_unordered_map_slot_t *n;
//
//				if (last == NULL){
//					map->table [i] = s->next;
//					n = s->next;
//				} else  {
//					last->next = s->next;
//					n = last->next;
//				}
//				dn_free (s);
//				map->in_use--;
//				count++;
//				s = n;
//			} else {
//				last = s;
//				s = s->next;
//			}
//		}
//	}
//	sanity_check (map);
//	if (count > 0)
//		rehash (map);
//	return count;
//}
//
//void
//dn_unordered_map_destroy (dn_unordered_map_t *map)
//{
//	if (DN_UNLIKELY(!map))
//		return;
//
//	for (int32_t i = 0; i < map->table_size; i++){
//		dn_unordered_map_slot_t *s, *next;
//		for (s = map->table [i]; s != NULL; s = next){
//			next = s->next;
//
//			if (map->key_destroy_func != NULL)
//				(*map->key_destroy_func)(s->key);
//			if (map->value_destroy_func != NULL)
//				(*map->value_destroy_func)(s->value);
//			dn_free (s);
//		}
//	}
//	dn_free (map->table);
//
//	dn_free (map);
//}
//
//void
//dn_unordered_map_print_stats (dn_unordered_map_t *map)
//{
//	int32_t max_chain_index, chain_size, max_chain_size;
//
//	max_chain_size = 0;
//	max_chain_index = -1;
//	for (int32_t i = 0; i < map->table_size; i++) {
//		chain_size = 0;
//		for (dn_unordered_map_slot_t *node = map->table [i]; node; node = node->next)
//			chain_size ++;
//		if (chain_size > max_chain_size) {
//			max_chain_size = chain_size;
//			max_chain_index = i;
//		}
//	}
//
//	printf ("Size: %d Table Size: %d Max Chain Length: %d at %d\n", map->in_use, map->table_size, max_chain_size, max_chain_index);
//}
//
//void
//dn_unordered_map_iter_init (
//	dn_unordered_map_iter_t *iter,
//	dn_unordered_map_t *map)
//{
//	memset (iter, 0, sizeof (dn_unordered_map_iter_t));
//	iter->map = map;
//	iter->index = -1;
//}
//
//bool
//dn_unordered_map_iter_next (
//	dn_unordered_map_iter_t *iter,
//	void **key,
//	void **value)
//{
//	if (DN_UNLIKELY(iter->index == -2))
//		return false;
//
//	if (!iter->slot) {
//		while (true) {
//			iter->index ++;
//			if (iter->index >= iter->map->table_size) {
//				iter->index = -2;
//				return false;
//			}
//			if (iter->map->table [iter->index])
//				break;
//		}
//		iter->slot = iter->map->table [iter->index];
//	}
//
//	if (key)
//		*key = iter->slot->key;
//	if (value)
//		*value = iter->slot->value;
//	iter->slot = iter->slot->next;
//
//	return true;
//}

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
