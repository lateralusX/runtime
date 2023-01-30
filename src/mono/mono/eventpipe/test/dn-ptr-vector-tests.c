// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-ptr-vector.h>

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_ptr_vector_memory_start_snapshot;
static _CrtMemState dn_ptr_vector_memory_end_snapshot;
static _CrtMemState dn_ptr_vector_memory_diff_snapshot;
#endif

#define POINTER_TO_INT32(v) ((int32_t)(ptrdiff_t)(v))
#define INT32_TO_POINTER(v) ((void *)(ptrdiff_t)(v))

static
RESULT
test_ptr_vector_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_ptr_vector_memory_start_snapshot);
#endif
	return OK;
}

/* Don't add more than 32 items to this please */
static const char *items [] = {
	"Apples", "Oranges", "Plumbs", "Goats", "Snorps", "Grapes",
	"Tickle", "Place", "Coffee", "Cookies", "Cake", "Cheese",
	"Tseng", "Holiday", "Avenue", "Smashing", "Water", "Toilet",
	NULL
};

static
dn_ptr_vector_t *
ptr_vector_alloc_and_fill (uint32_t *item_count)
{
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	int32_t i;

	for(i = 0; items [i] != NULL; i++) {
		dn_ptr_vector_push_back (vector, (void *)items [i]);
	}

	if (item_count != NULL) {
		*item_count = i;
	}

	return vector;
}

static
uint32_t
ptr_vector_guess_capacity (uint32_t capacity)
{
	return ((capacity + (capacity >> 1) + 63) & ~63);
}

static
RESULT
test_ptr_vector_alloc (void)
{
	dn_ptr_vector_t *vector;
	uint32_t i;

	vector = ptr_vector_alloc_and_fill (&i);

	if (dn_ptr_vector_capacity (vector) != ptr_vector_guess_capacity (vector->size)) {
		return FAILED ("capacity should be %d, but it is %d",
			ptr_vector_guess_capacity (vector->size), dn_ptr_vector_capacity (vector));
	}

	if (vector->size != i) {
		return FAILED ("expected %d node(s) in the vector", i);
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_free (void)
{
	int32_t v = 27;
	dn_ptr_vector_t *vector = NULL;

	vector = dn_ptr_vector_alloc ();
	if (vector->size != 0)
		return FAILED ("vector size didn't match #1");

	dn_ptr_vector_free (vector);

	vector = dn_ptr_vector_alloc ();
	if (vector->size != 0)
		return FAILED ("vector size didn't match #2");

	dn_ptr_vector_push_back (vector, &v);

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_alloc_capacity (void)
{
	dn_ptr_vector_t *vector = NULL;

	vector = dn_ptr_vector_alloc_capacity (ARRAY_SIZE (items));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	if (dn_ptr_vector_capacity (vector) < ARRAY_SIZE (items))
		return FAILED ("capacity didn't match");

	void **data = dn_ptr_vector_data (vector);
	for (int32_t i = 0; i < ARRAY_SIZE (items); ++i) {
		dn_ptr_vector_push_back (vector, (void *)items [i]);
		if (dn_ptr_vector_data (vector) != data)
			return FAILED ("vector pre-alloc failed");
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_for_iterate (void)
{
	dn_ptr_vector_t *vector = ptr_vector_alloc_and_fill (NULL);
	uint32_t i = 0;

	DN_PTR_VECTOR_FOREACH_BEGIN (vector, char *, item) {
		if (item != items[i]) {
			return FAILED (
				"expected item at %d to be %s, but it was %s",
				i, items[i], item);
		}
		i++;
	} DN_PTR_VECTOR_FOREACH_END;

	dn_ptr_vector_free (vector);

	return OK;
}

static int32_t foreach_iterate_index = 0;
static char *foreach_iterate_error = NULL;

static
void
DN_CALLBACK_CALLTYPE
ptr_vector_foreach_callback (
	void *data,
	void *user_data)
{
	char *item = *((char **)data);
	const char *item_cmp = items [foreach_iterate_index++];

	if (foreach_iterate_error != NULL) {
		return;
	}

	if (item != item_cmp) {
		foreach_iterate_error = FAILED (
			"expected item at %d to be %s, but it was %s",
				foreach_iterate_index - 1, item_cmp, item);
	}
}

static
RESULT
test_ptr_vector_foreach_iterate (void)
{
	dn_ptr_vector_t *vector = ptr_vector_alloc_and_fill (NULL);

	foreach_iterate_index = 0;
	foreach_iterate_error = NULL;

	dn_ptr_vector_for_each (vector, ptr_vector_foreach_callback, vector);

	dn_ptr_vector_free (vector);

	return foreach_iterate_error;
}

static
RESULT
test_ptr_vector_resize (void)
{
	dn_ptr_vector_t *vector= dn_ptr_vector_alloc ();
	uint32_t i, grow_length = 50;

	dn_ptr_vector_push_back (vector, (void *)items [0]);
	dn_ptr_vector_push_back (vector, (void *)items [1]);
	dn_ptr_vector_resize (vector, grow_length);

	if (vector->size != grow_length) {
		return FAILED ("vector size should be 50, it is %d", vector->size);
	} else if (*dn_ptr_vector_index (vector, 0) != items[0]) {
		return FAILED ("item 0 was overwritten, should be %s", items[0]);
	} else if (*dn_ptr_vector_index (vector, 1) != items[1]) {
		return FAILED ("item 1 was overwritten, should be %s", items[1]);
	}

	for (i = 2; i < vector->size; i++) {
		if (*dn_ptr_vector_index (vector, i) != NULL) {
			return FAILED ("item %d is not NULL, it is %p", i, vector->data[i]);
		}
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_erase (void)
{
	dn_ptr_vector_t *vector;
	uint32_t i;

	vector = ptr_vector_alloc_and_fill (&i);

	dn_ptr_vector_erase (vector, 0);
	if (*dn_ptr_vector_index (vector, 0) != items [1]) {
		return FAILED ("first item is not %s, it is %s", items [1],
			*dn_ptr_vector_index (vector, 0));
	}

	dn_ptr_vector_erase (vector, vector->size - 1);

	if (*dn_ptr_vector_index (vector, vector->size - 1) != items [vector->size]) {
		return FAILED ("last item is not %s, it is %s",
			items [vector->size - 2], *dn_ptr_vector_index (vector, vector->size - 1));
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_erase_fast (void)
{
	dn_ptr_vector_t *vector;
	uint32_t i;

	vector = ptr_vector_alloc_and_fill (&i);

	dn_ptr_vector_erase_fast (vector, 0);
	if (*dn_ptr_vector_index (vector, 0) != items [vector->size]) {
		return FAILED ("first item is not %s, it is %s", items [vector->size],
			*dn_ptr_vector_index (vector, 0));
	}

	dn_ptr_vector_erase_fast (vector, vector->size - 1);
	if (*dn_ptr_vector_index (vector, vector->size - 1) != items [vector->size - 1]) {
		return FAILED ("last item is not %s, it is %s",
			items [vector->size - 1], *dn_ptr_vector_index (vector, vector->size - 1));
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_erase_if (void)
{
	dn_ptr_vector_t *vector;
	uint32_t i;

	vector = ptr_vector_alloc_and_fill (&i);

	dn_ptr_vector_erase_if (vector, (void *)items [7]);

	if (!dn_ptr_vector_erase_if (vector, (void *)items [4])) {
		return FAILED ("item %s not removed", items [4]);
	}

	if (dn_ptr_vector_erase_if (vector, (void *)items [4])) {
		return FAILED ("item %s still in vector after removal", items [4]);
	}

	if (*dn_ptr_vector_index (vector, vector->size - 1) != items [vector->size + 1]) {
		return FAILED ("last item in vector not correct");
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_erase_fast_if (void)
{
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();

	static char * const letters [] = { (char*)"A", (char*)"B", (char*)"C", (char*)"D", (char*)"E" };

	if (dn_ptr_vector_erase_fast_if (vector, NULL))
		return FAILED ("removing NULL succeeded");

	dn_ptr_vector_push_back (vector, letters [0]);
	if (!dn_ptr_vector_erase_fast_if (vector, letters [0]) || vector->size != 0)
		return FAILED ("removing last element failed");

	dn_ptr_vector_push_back (vector, letters [0]);
	dn_ptr_vector_push_back (vector, letters [1]);
	dn_ptr_vector_push_back (vector, letters [2]);
	dn_ptr_vector_push_back (vector, letters [3]);
	dn_ptr_vector_push_back (vector, letters [4]);

	if (!dn_ptr_vector_erase_fast_if (vector, letters [0]) || vector->size != 4)
		return FAILED ("removing first element failed");

	if (*dn_ptr_vector_index (vector, 0) != letters [4])
		return FAILED ("first element wasn't replaced with last upon removal");

	if (dn_ptr_vector_erase_fast_if (vector, letters [0]))
		return FAILED ("succeeded removing a non-existing element");

	if (!dn_ptr_vector_erase_fast_if (vector, letters [3]) || vector->size != 3)
		return FAILED ("failed removing \"D\"");

	if (!dn_ptr_vector_erase_fast_if (vector, letters [1]) || vector->size != 2)
		return FAILED ("failed removing \"B\"");

	if (*dn_ptr_vector_index (vector, 0) != letters [4] || *dn_ptr_vector_index (vector, 1) != letters [2])
		return FAILED ("last two elements are wrong");

	dn_ptr_vector_free(vector);

	return OK;
}

static
RESULT
test_ptr_vector_capacity (void)
{
	uint32_t size;
	dn_ptr_vector_t *vector = ptr_vector_alloc_and_fill (&size);

	if (dn_ptr_vector_capacity (vector) < size)
		return FAILED ("invalid vector capacity #1");

	if (dn_ptr_vector_capacity (vector) < vector->size)
		return FAILED ("invalid arvectorray capacity #2");

	uint32_t capacity = dn_ptr_vector_capacity (vector);

	void *value0 = *dn_ptr_vector_index (vector, 0);
	dn_ptr_vector_erase (vector, 0);

	void *value1 = *dn_ptr_vector_index (vector, 1);
	dn_ptr_vector_erase (vector, 1);

	void *value2 = *dn_ptr_vector_index (vector, 2);
	dn_ptr_vector_erase (vector, 2);

	if (dn_ptr_vector_capacity (vector) != capacity)
		return FAILED ("invalid vector capacity #3");

	dn_ptr_vector_push_back (vector, value0);
	dn_ptr_vector_push_back (vector, value1);
	dn_ptr_vector_push_back (vector, value2);

	if (dn_ptr_vector_capacity (vector) != capacity)
		return FAILED ("invalid vector capacity #4");

	dn_ptr_vector_free (vector);

	vector = dn_ptr_vector_alloc_capacity (ARRAY_SIZE (items));
	if (vector->size != 0)
		return FAILED ("vector len didn't match");

	if (dn_ptr_vector_capacity (vector) < ARRAY_SIZE (items))
		return FAILED ("invalid vector capacity #5");

	capacity = dn_ptr_vector_capacity (vector);
	for (int32_t i = 0; i < ARRAY_SIZE (items); ++i)
		dn_ptr_vector_push_back (vector, (void *)items [i]);

	if (dn_ptr_vector_capacity (vector) != capacity)
		return FAILED ("invalid vector capacity #6");

	dn_ptr_vector_free (vector);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
ptr_vector_free_fini_func(void *data)
{
	free (*(char **)data);
}

static
void
DN_CALLBACK_CALLTYPE
ptr_vector_clear_fini_func(void *data)
{
	(**((uint32_t **)data))++;
}

static
RESULT
test_ptr_vector_foreach_free (void)
{
	int32_t count = 0;
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);

	dn_ptr_vector_for_each_fini (vector, ptr_vector_clear_fini_func);
	dn_ptr_vector_free (vector);

	if (count != 2)
		return FAILED ("callback called incorrect number of times");

	vector = dn_ptr_vector_alloc ();
	
	dn_ptr_vector_push_back (vector, malloc (10));
	dn_ptr_vector_push_back (vector, malloc (100));
	
	dn_ptr_vector_for_each_fini (vector, ptr_vector_free_fini_func);
	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_clear (void)
{
	uint32_t count = 0;
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	if (vector->size != 0)
		return FAILED ("vector size didn't match #1");

	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);

	dn_ptr_vector_clear (vector);
	if (vector->size != 0)
		return FAILED ("vector size didn't match #2");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_foreach_clear (void)
{
	uint32_t count = 0;
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	if (vector->size != 0)
		return FAILED ("vector size didn't match #1");

	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);
	dn_ptr_vector_push_back (vector, &count);

	uint32_t capacity = dn_ptr_vector_capacity (vector);

	dn_ptr_vector_for_each_fini (vector, ptr_vector_clear_fini_func);
	dn_ptr_vector_clear (vector);

	if (vector->size != 0)
		return FAILED ("vector size didn't match #2");

	if (dn_ptr_vector_capacity (vector) != capacity)
		return FAILED ("incorrect vector capacity");

	if (count != 5)
		return FAILED ("allback called incorrect number of times");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_foreach_it (void)
{
	uint32_t count = 0;
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_ptr_vector_push_back (vector, INT32_TO_POINTER (i));

	DN_PTR_VECTOR_FOREACH_BEGIN (vector, uint32_t *, value) {
		if (POINTER_TO_INT32 (value) != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_VECTOR_FOREACH_END;

	if (count != dn_ptr_vector_size (vector))
		return FAILED ("foreach iterator failed #2");

	dn_ptr_vector_free (vector);
	return OK;
}

static
RESULT
test_ptr_vector_foreach_rev_it (void)
{
	uint32_t count = 100;
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_ptr_vector_push_back (vector, INT32_TO_POINTER (i));

	DN_PTR_VECTOR_FOREACH_RBEGIN (vector, uint32_t *, value) {
		if (POINTER_TO_INT32 (value) != count - 1)
			return FAILED ("foreach reverse iterator failed #1");
		count--;
	} DN_PTR_VECTOR_FOREACH_END;

	if (count != 0)
		return FAILED ("foreach reverse iterator failed #2");

	dn_ptr_vector_free (vector);
	return OK;
}

static
int32_t
DN_CALLBACK_CALLTYPE
ptr_vector_sort_compare (
	const void *a,
	const void *b)
{
	char *stra = *(char **) a;
	char *strb = *(char **) b;
	return strcmp(stra, strb);
}

static
RESULT
test_ptr_vector_sort (void)
{
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();
	uint32_t i;

	static char * const letters [] = { (char*)"A", (char*)"B", (char*)"C", (char*)"D", (char*)"E" };

	dn_ptr_vector_push_back (vector, letters [0]);
	dn_ptr_vector_push_back (vector, letters [1]);
	dn_ptr_vector_push_back (vector, letters [2]);
	dn_ptr_vector_push_back (vector, letters [3]);
	dn_ptr_vector_push_back (vector, letters [4]);

	dn_ptr_vector_sort (vector, ptr_vector_sort_compare);

	for (i = 0; i < vector->size; i++) {
		if (vector->data [i] != letters [i]) {
			return FAILED ("vector out of order, expected %s got %s at position %d",
				letters [i], (char *) vector->data [i], i);
		}
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_find (void)
{
	dn_ptr_vector_t *vector = dn_ptr_vector_alloc ();

	static char * const letters [] = { (char*)"A", (char*)"B", (char*)"C", (char*)"D", (char*)"E" };

	dn_ptr_vector_push_back (vector, letters [0]);
	dn_ptr_vector_push_back (vector, letters [1]);
	dn_ptr_vector_push_back (vector, letters [2]);
	dn_ptr_vector_push_back (vector, letters [3]);
	dn_ptr_vector_push_back (vector, letters [4]);

	if (dn_ptr_vector_find (vector, letters [0]) == dn_ptr_vector_size (vector))
		return FAILED ("failed to find value #1");

	if (dn_ptr_vector_find (vector, letters [1]) == dn_ptr_vector_size (vector))
		return FAILED ("failed to find value #2");

	if (dn_ptr_vector_find (vector, letters [2]) == dn_ptr_vector_size (vector))
		return FAILED ("failed to find value #3");

	if (dn_ptr_vector_find (vector, letters [3]) == dn_ptr_vector_size (vector))
		return FAILED ("failed to find value #4");

	if (dn_ptr_vector_find (vector, letters [4]) == dn_ptr_vector_size (vector))
		return FAILED ("failed to find value #5");

	if (dn_ptr_vector_find (vector, NULL) != dn_ptr_vector_size (vector))
		return FAILED ("find failed #6");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_default_local_alloc (void)
{
	DN_DEFAULT_LOCAL_ALLOCATOR (allocator, dn_ptr_vector_default_local_allocator_byte_size);

	uint32_t init_capacity = dn_ptr_vector_buffer_capacity (dn_ptr_vector_default_local_allocator_byte_size);
	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc ((dn_allocator_t *)&allocator);
	if (!vector)
		return FAILED ("failed vector custom alloc");

	for (uint32_t i = 0; i < init_capacity; ++i) {
		for (uint32_t j = 0; j < ARRAY_SIZE (items); ++j) {
			if (!dn_ptr_vector_push_back (vector, (void *)items [j]))
				return FAILED ("failed vector push_back using custom alloc");
		}
	}

	for (uint32_t i = 0; i < init_capacity; ++i) {
		for (uint32_t j = 0; j < ARRAY_SIZE (items); ++j) {
			if (*dn_ptr_vector_index (vector, ARRAY_SIZE (items) * i + j) != items [j])
				return FAILED ("vector realloc failure using default local alloc");
		}
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
bool
ptr_vector_owned_by_fixed_allocator (
	dn_allocator_fixed_t *allocator,
	dn_ptr_vector_t *vector)
{
	return	allocator->_data._begin <= (void *)dn_ptr_vector_data (vector) &&
		allocator->_data._end > (void *)dn_ptr_vector_data (vector);
}

static
RESULT
test_ptr_vector_local_alloc (void)
{
	uint8_t buffer [1024];
	dn_allocator_fixed_or_malloc_t allocator;

	dn_allocator_fixed_or_malloc_init (&allocator, buffer, ARRAY_SIZE (buffer));
	memset (buffer, 0, ARRAY_SIZE (buffer));

	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc ((dn_allocator_t *)&allocator);
	if (!vector)
		return FAILED ("failed vector custom alloc");

	// All should fit in fixed allocator.
	for (uint32_t i = 0; i < ARRAY_SIZE (items); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i]))
			return FAILED ("failed vector push_back using custom alloc #1");
	}

	if (!ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("custom alloc using fixed allocator failed");

	// Make sure we run out of fixed allocator memory, should switch to dynamic allocator.
	for (uint32_t i = 0; i < ARRAY_SIZE (buffer); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc #2");
	}

	if (ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("custom alloc using dynamic allocator failed");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_local_alloc_capacity (void)
{
	uint8_t buffer [dn_ptr_vector_default_local_allocator_byte_size];
	dn_allocator_fixed_or_malloc_t allocator;

	dn_allocator_fixed_or_malloc_init (&allocator, buffer, dn_ptr_vector_default_local_allocator_byte_size);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	uint32_t init_capacity = dn_ptr_vector_buffer_capacity (dn_ptr_vector_default_local_allocator_byte_size);
	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc");

	if (dn_ptr_vector_capacity (vector) != dn_ptr_vector_default_local_allocator_capacity_size)
		return FAILED ("default local vector should have %d in capacity #1", dn_ptr_vector_default_local_allocator_capacity_size);

	// Make sure pre-allocted fixed allocator is used.
	if (!ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("custom alloc using fixed allocator failed #1");

	// Add pre-allocated amount of items, should fit into fixed buffer.
	for (uint32_t i = 0; i < dn_ptr_vector_capacity (vector); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc");
	}

	if (dn_ptr_vector_capacity (vector) != dn_ptr_vector_default_local_allocator_capacity_size)
		return FAILED ("default local vector should have %d in capacity #2", dn_ptr_vector_default_local_allocator_capacity_size);

	// Make sure pre-allocted fixed allocator is used without reallocs (would cause OOM and switch to dynamic).
	if (!ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("custom alloc using fixed allocator failed #2");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_fixed_alloc_capacity (void)
{
	uint8_t buffer [dn_ptr_vector_default_local_allocator_byte_size];
	dn_allocator_fixed_t allocator;

	dn_allocator_fixed_init (&allocator, buffer, dn_ptr_vector_default_local_allocator_byte_size);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	uint32_t init_capacity = dn_ptr_vector_buffer_capacity (dn_ptr_vector_default_local_allocator_byte_size);
	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc");

	// Add pre-allocated amount of items, should fit into fixed buffer.
	for (uint32_t i = 0; i < dn_ptr_vector_capacity (vector); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc");
	}

	// Adding one more should hit OOM.
	if (dn_ptr_vector_push_back (vector, (void *)items [0]))
		return FAILED ("vector push_back failed to triggered OOM");

	// Make room for on more item.
	dn_ptr_vector_erase_if (vector, (void *)items [0]);

	// Adding one more should not hit OOM.
	if (!dn_ptr_vector_push_back (vector, (void *)items [0]))
		return FAILED ("vector push_back triggered OOM");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_fixed_or_malloc_alloc_capacity (void)
{
	uint8_t buffer [dn_ptr_vector_default_local_allocator_byte_size];
	dn_allocator_fixed_or_malloc_t allocator;

	dn_allocator_fixed_or_malloc_init (&allocator, buffer, dn_ptr_vector_default_local_allocator_byte_size);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	uint32_t init_capacity = dn_ptr_vector_buffer_capacity (dn_ptr_vector_default_local_allocator_byte_size);
	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc");

	// Add pre-allocated amount of items, should fit into fixed allocator.
	for (uint32_t i = 0; i < dn_ptr_vector_capacity (vector); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc #1");
	}

	// Make sure pre-allocted fixed allocator is used.
	if (!ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("custom alloc using fixed allocator failed");

	// Adding one more should not hit OOM.
	if (!dn_ptr_vector_push_back (vector, (void *)items [0]))
		return FAILED ("failed vector push_back using custom alloc #2");

	if (dn_ptr_vector_capacity (vector) <= init_capacity)
		return FAILED ("unexpected vector capacity #1");

	init_capacity = dn_ptr_vector_capacity (vector);

	// Make room for on more item.
	dn_ptr_vector_erase_if (vector, (void *)items [0]);

	if (dn_ptr_vector_capacity (vector) < init_capacity)
		return FAILED ("unexpected vector capacity #2");

	// Validate continious use of dynamic allocator.
	if (ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("unexpected switch to fixed allocator");

	// Adding one more should not hit OOM.
	if (!dn_ptr_vector_push_back (vector, (void *)items [0]))
		return FAILED ("failed vector push_back using custom alloc #3");

	if (dn_ptr_vector_capacity (vector) < init_capacity)
		return FAILED ("unexpected vector capacity #3");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_fixed_reset_alloc_capacity (void)
{
	uint8_t buffer [dn_ptr_vector_default_local_allocator_byte_size];
	dn_allocator_fixed_t allocator;

	dn_allocator_fixed_init (&allocator, buffer, dn_ptr_vector_default_local_allocator_byte_size);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	uint32_t init_capacity = dn_ptr_vector_buffer_capacity (dn_ptr_vector_default_local_allocator_byte_size);
	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc #1");

	// Add pre-allocated amount of items, should fit into fixed allocator.
	for (uint32_t i = 0; i < dn_ptr_vector_capacity (vector); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc");
	}

	// Adding one more should hit OOM.
	if (dn_ptr_vector_push_back (vector, (void *)items [0]))
		return FAILED ("vector push_back failed to triggered OOM");

	dn_ptr_vector_free (vector);

	// Reset fixed allocator.
	dn_allocator_fixed_reset (&allocator);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc #2");

	// Add pre-allocated amount of items, should fit into fixed buffer.
	for (uint32_t i = 0; i < dn_ptr_vector_capacity (vector); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc");
	}

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_fixed_or_malloc_reset_alloc_capacity (void)
{
	uint8_t buffer [dn_ptr_vector_default_local_allocator_byte_size];
	dn_allocator_fixed_or_malloc_t allocator;

	dn_allocator_fixed_or_malloc_init (&allocator, buffer, dn_ptr_vector_default_local_allocator_byte_size);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	uint32_t init_capacity = dn_ptr_vector_buffer_capacity (dn_ptr_vector_default_local_allocator_byte_size);
	dn_ptr_vector_t *vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc #1");

	// Add pre-allocated amount of items, should fit into fixed allocator.
	for (uint32_t i = 0; i < dn_ptr_vector_capacity (vector); ++i) {
		if (!dn_ptr_vector_push_back (vector, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed vector push_back using custom alloc #1");
	}

	// Adding one more should not hit OOM but switch to dynamic allocator.
	if (!dn_ptr_vector_push_back (vector, (void *)items [0]))
		return FAILED ("failed vector push_back using custom alloc #2");

	// Validate use of dynamic allocator.
	if (ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("unexpected switch to fixed allocator #1");

	dn_ptr_vector_free (vector);

	vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc #2");

	// Validate use of dynamic allocator.
	if (ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("unexpected switch to fixed allocator #2");

	dn_ptr_vector_free (vector);

	// Reset fixed part of allocator.
	dn_allocator_fixed_or_malloc_reset (&allocator);
	memset (buffer, 0, dn_ptr_vector_default_local_allocator_byte_size);

	vector = dn_ptr_vector_custom_alloc_capacity ((dn_allocator_t *)&allocator, init_capacity);
	if (!vector)
		return FAILED ("failed vector custom alloc #2");

	// Validate use of fixed allocator.
	if (!ptr_vector_owned_by_fixed_allocator ((dn_allocator_fixed_t *)&allocator, vector))
		return FAILED ("custom alloc using fixed allocator failed");

	dn_ptr_vector_free (vector);

	return OK;
}

static
RESULT
test_ptr_vector_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_ptr_vector_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_ptr_vector_memory_diff_snapshot, &dn_ptr_vector_memory_start_snapshot, &dn_ptr_vector_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_ptr_vector_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_ptr_vector_tests [] = {
	{"test_ptr_vector_setup", test_ptr_vector_setup},
	{"test_ptr_vector_alloc", test_ptr_vector_alloc},
	{"test_ptr_vector_free", test_ptr_vector_free},
	{"test_ptr_vector_alloc_capacity", test_ptr_vector_alloc_capacity},
	{"test_ptr_vector_for_iterate", test_ptr_vector_for_iterate},
	{"test_ptr_vector_foreach_iterate", test_ptr_vector_foreach_iterate},
	{"test_ptr_vector_resize", test_ptr_vector_resize},
	{"test_ptr_vector_erase", test_ptr_vector_erase},
	{"test_ptr_vector_erase_fast", test_ptr_vector_erase_fast},
	{"test_ptr_vector_erase_if", test_ptr_vector_erase_if},
	{"test_ptr_vector_erase_fast_if", test_ptr_vector_erase_fast_if},
	{"test_ptr_vector_capacity", test_ptr_vector_capacity},
	{"test_ptr_vector_foreach_free", test_ptr_vector_foreach_free},
	{"test_ptr_vector_clear", test_ptr_vector_clear},
	{"test_ptr_vector_foreach_clear", test_ptr_vector_foreach_clear},
	{"test_ptr_vector_foreach_it", test_ptr_vector_foreach_it},
	{"test_ptr_vector_foreach_rev_it", test_ptr_vector_foreach_rev_it},
	{"test_ptr_vector_sort", test_ptr_vector_sort},
	{"test_ptr_vector_find", test_ptr_vector_find},
	{"test_ptr_vector_default_local_alloc", test_ptr_vector_default_local_alloc},
	{"test_ptr_vector_local_alloc", test_ptr_vector_local_alloc},
	{"test_ptr_vector_local_alloc_capacity", test_ptr_vector_local_alloc_capacity},
	{"test_ptr_vector_fixed_alloc_capacity", test_ptr_vector_fixed_alloc_capacity},
	{"test_ptr_vector_fixed_or_malloc_alloc_capacity", test_ptr_vector_fixed_or_malloc_alloc_capacity},
	{"test_ptr_vector_fixed_reset_alloc_capacity", test_ptr_vector_fixed_reset_alloc_capacity},
	{"test_ptr_vector_fixed_or_malloc_reset_alloc_capacity", test_ptr_vector_fixed_or_malloc_reset_alloc_capacity},
	{"test_ptr_vector_teardown", test_ptr_vector_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_ptr_vector_tests_init, dn_ptr_vector_tests)
