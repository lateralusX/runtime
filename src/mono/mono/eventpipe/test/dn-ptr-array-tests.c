#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-ptr-array-ex.h>

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_ptr_array_memory_start_snapshot;
static _CrtMemState dn_ptr_array_memory_end_snapshot;
static _CrtMemState dn_ptr_array_memory_diff_snapshot;
#endif

#define POINTER_TO_INT32(v) ((int32_t)(ptrdiff_t)(v))
#define INT32_TO_POINTER(v) ((void *)(ptrdiff_t)(v))

static
RESULT
test_ptr_array_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_ptr_array_memory_start_snapshot);
#endif
	return OK;
}

/* Redefine the private structure only to verify proper allocations */
typedef struct _dn_ptr_array_priv_t {
	void *pdata;
	uint32_t len;
	uint32_t size;
} dn_ptr_array_priv_t;

/* Don't add more than 32 items to this please */
static const char *items [] = {
	"Apples", "Oranges", "Plumbs", "Goats", "Snorps", "Grapes",
	"Tickle", "Place", "Coffee", "Cookies", "Cake", "Cheese",
	"Tseng", "Holiday", "Avenue", "Smashing", "Water", "Toilet",
	NULL
};

static
dn_ptr_array_t *
ptr_array_alloc_and_fill (uint32_t *item_count)
{
	dn_ptr_array_t *array = dn_ptr_array_new ();
	int32_t i;

	for(i = 0; items [i] != NULL; i++) {
		dn_ptr_array_add (array, (void *)items [i]);
	}

	if (item_count != NULL) {
		*item_count = i;
	}

	return array;
}

static
uint32_t
ptr_array_guess_size(uint32_t length)
{
	uint32_t size = 1;

	while (size < length) {
		size <<= 1;
	}

	return size;
}

static
RESULT
test_ptr_array_new (void)
{
	dn_ptr_array_priv_t *array;
	uint32_t i;

	array = (dn_ptr_array_priv_t *)ptr_array_alloc_and_fill (&i);

	if (array->size != ptr_array_guess_size (array->len)) {
		return FAILED ("size should be %d, but it is %d",
			ptr_array_guess_size (array->len), array->size);
	}

	if (array->len != i) {
		return FAILED ("expected %d node(s) in the array", i);
	}

	dn_ptr_array_free ((dn_ptr_array_t *)array, true);

	return OK;
}

static
RESULT
test_ptr_array_free (void)
{
	int32_t v = 27;
	dn_ptr_array_t *array = NULL;

	array = dn_ptr_array_new ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	dn_ptr_array_free (array, true);

	array = dn_ptr_array_new ();
	if (array->len != 0)
		return FAILED ("array len didn't match #2");

	dn_ptr_array_add (array, &v);

	int32_t **data = (int32_t **)dn_ptr_array_free (array, false);
	if (*data [0] != 27)
		return FAILED ("free didn't preserve segment");

	dn_ptr_array_free_segment ((void **)data);

	return OK;
}

static
RESULT
test_ptr_array_sized_new (void)
{
	dn_ptr_array_t *array = NULL;

	array = dn_ptr_array_sized_new (ARRAY_SIZE (items));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (dn_ptr_array_capacity (array) < ARRAY_SIZE (items))
		return FAILED ("capacity didn't match");

	void *data = array->data;
	for (int32_t i = 0; i < ARRAY_SIZE (items); ++i) {
		dn_ptr_array_add (array, (void *)items [i]);
		if (array->data != data)
			return FAILED ("array pre-alloc failed");
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_for_iterate (void)
{
	dn_ptr_array_t *array = ptr_array_alloc_and_fill (NULL);
	uint32_t i;

	for (i = 0; i < array->len; i++) {
		char *item = (char *)dn_ptr_array_index (array, i);
		if (item != items[i]) {
			return FAILED (
				"expected item at %d to be %s, but it was %s",
				i, items[i], item);
		}
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static int32_t foreach_iterate_index = 0;
static char *foreach_iterate_error = NULL;

static
void
DN_CALLBACK_CALLTYPE
ptr_array_foreach_callback (
	void *data,
	void *user_data)
{
	char *item = (char *)data;
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
test_ptr_array_foreach_iterate (void)
{
	dn_ptr_array_t *array = ptr_array_alloc_and_fill (NULL);

	foreach_iterate_index = 0;
	foreach_iterate_error = NULL;

	dn_ptr_array_foreach (array, ptr_array_foreach_callback, array);

	dn_ptr_array_free (array, true);

	return foreach_iterate_error;
}

static
RESULT
test_ptr_array_set_size (void)
{
	dn_ptr_array_t *array = dn_ptr_array_new ();
	uint32_t i, grow_length = 50;

	dn_ptr_array_add (array, (void *)items [0]);
	dn_ptr_array_add (array, (void *)items [1]);
	dn_ptr_array_set_size (array, grow_length);

	if (array->len != grow_length) {
		return FAILED ("array length should be 50, it is %d", array->len);
	} else if (array->data[0] != items[0]) {
		return FAILED ("item 0 was overwritten, should be %s", items[0]);
	} else if (array->data[1] != items[1]) {
		return FAILED ("item 1 was overwritten, should be %s", items[1]);
	}

	for (i = 2; i < array->len; i++) {
		if (array->data[i] != NULL) {
			return FAILED ("item %d is not NULL, it is %p", i, array->data[i]);
		}
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_remove_index (void)
{
	dn_ptr_array_t *array;
	uint32_t i;

	array = ptr_array_alloc_and_fill (&i);

	dn_ptr_array_remove_index (array, 0);
	if (array->data [0] != items [1]) {
		return FAILED ("first item is not %s, it is %s", items [1],
			array->data [0]);
	}

	dn_ptr_array_remove_index (array, array->len - 1);

	if (array->data [array->len - 1] != items [array->len]) {
		return FAILED ("fast item is not %s, it is %s",
			items [array->len - 2], array->data [array->len - 1]);
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_remove_index_fast (void)
{
	dn_ptr_array_t *array;
	uint32_t i;

	array = ptr_array_alloc_and_fill (&i);

	dn_ptr_array_remove_index_fast (array, 0);
	if (array->data [0] != items [array->len]) {
		return FAILED ("first item is not %s, it is %s", items [array->len],
			array->data [0]);
	}

	dn_ptr_array_remove_index_fast (array, array->len - 1);
	if (array->data [array->len - 1] != items [array->len - 1]) {
		return FAILED ("last item is not %s, it is %s",
			items [array->len - 1], array->data [array->len - 1]);
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_remove (void)
{
	dn_ptr_array_t *array;
	uint32_t i;

	array = ptr_array_alloc_and_fill (&i);

	dn_ptr_array_remove (array, (void *)items [7]);

	if (!dn_ptr_array_remove (array, (void *)items [4])) {
		return FAILED ("item %s not removed", items [4]);
	}

	if (dn_ptr_array_remove(array, (void *)items [4])) {
		return FAILED ("item %s still in array after removal", items [4]);
	}

	if (array->data [array->len - 1] != items [array->len + 1]) {
		return FAILED ("last item in array not correct");
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static
int32_t
DN_CALLBACK_CALLTYPE
ptr_array_sort_compare (
	const void *a,
	const void *b)
{
	char *stra = *(char **) a;
	char *strb = *(char **) b;
	return strcmp(stra, strb);
}

static
RESULT
test_ptr_array_sort (void)
{
	dn_ptr_array_t *array = dn_ptr_array_new ();
	uint32_t i;

	static char * const letters [] = { (char*)"A", (char*)"B", (char*)"C", (char*)"D", (char*)"E" };

	dn_ptr_array_add (array, letters [0]);
	dn_ptr_array_add (array, letters [1]);
	dn_ptr_array_add (array, letters [2]);
	dn_ptr_array_add (array, letters [3]);
	dn_ptr_array_add (array, letters [4]);

	dn_ptr_array_sort (array, ptr_array_sort_compare);

	for (i = 0; i < array->len; i++) {
		if (array->data [i] != letters [i]) {
			return FAILED ("array out of order, expected %s got %s at position %d",
				letters [i], (char *) array->data [i], i);
		}
	}

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_remove_fast (void)
{
	dn_ptr_array_t *array = dn_ptr_array_new ();

	static char * const letters [] = { (char*)"A", (char*)"B", (char*)"C", (char*)"D", (char*)"E" };

	if (dn_ptr_array_remove_fast (array, NULL))
		return FAILED ("removing NULL succeeded");

	dn_ptr_array_add (array, letters [0]);
	if (!dn_ptr_array_remove_fast (array, letters [0]) || array->len != 0)
		return FAILED ("removing last element failed");

	dn_ptr_array_add (array, letters [0]);
	dn_ptr_array_add (array, letters [1]);
	dn_ptr_array_add (array, letters [2]);
	dn_ptr_array_add (array, letters [3]);
	dn_ptr_array_add (array, letters [4]);

	if (!dn_ptr_array_remove_fast (array, letters [0]) || array->len != 4)
		return FAILED ("removing first element failed");

	if (array->data [0] != letters [4])
		return FAILED ("first element wasn't replaced with last upon removal");

	if (dn_ptr_array_remove_fast (array, letters [0]))
		return FAILED ("succeeded removing a non-existing element");

	if (!dn_ptr_array_remove_fast (array, letters [3]) || array->len != 3)
		return FAILED ("failed removing \"D\"");

	if (!dn_ptr_array_remove_fast (array, letters [1]) || array->len != 2)
		return FAILED ("failed removing \"B\"");

	if (array->data [0] != letters [4] || array->data [1] != letters [2])
		return FAILED ("last two elements are wrong");

	dn_ptr_array_free(array, true);

	return OK;
}

static
RESULT
test_ptr_array_capacity (void)
{
	uint32_t size;
	dn_ptr_array_t *array = ptr_array_alloc_and_fill (&size);

	if (dn_ptr_array_capacity (array) < size)
		return FAILED ("invalid array capacity #1");

	if (dn_ptr_array_capacity (array) < array->len)
		return FAILED ("invalid array capacity #2");

	uint32_t capacity = dn_ptr_array_capacity (array);

	void *value0 = dn_ptr_array_remove_index (array, 0);
	void *value1 = dn_ptr_array_remove_index (array, 1);
	void *value2 = dn_ptr_array_remove_index (array, 2);

	if (dn_ptr_array_capacity (array) != capacity)
		return FAILED ("invalid array capacity #3");

	dn_ptr_array_add (array, value0);
	dn_ptr_array_add (array, value1);
	dn_ptr_array_add (array, value2);

	if (dn_ptr_array_capacity (array) != capacity)
		return FAILED ("invalid array capacity #4");

	dn_ptr_array_free (array, true);

	array = dn_ptr_array_sized_new (ARRAY_SIZE (items));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (dn_ptr_array_capacity (array) < ARRAY_SIZE (items))
		return FAILED ("invalid array capacity #5");

	capacity = dn_ptr_array_capacity (array);
	for (int32_t i = 0; i < ARRAY_SIZE (items); ++i)
		dn_ptr_array_add (array, (void *)items [i]);

	if (dn_ptr_array_capacity (array) != capacity)
		return FAILED ("invalid array capacity #6");

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_find (void)
{
	dn_ptr_array_t *array = dn_ptr_array_new ();
	uint32_t i;

	static char * const letters [] = { (char*)"A", (char*)"B", (char*)"C", (char*)"D", (char*)"E" };

	dn_ptr_array_add (array, letters [0]);
	dn_ptr_array_add (array, letters [1]);
	dn_ptr_array_add (array, letters [2]);
	dn_ptr_array_add (array, letters [3]);
	dn_ptr_array_add (array, letters [4]);

	if (!dn_ptr_array_find (array, letters [0], &i) || i != 0)
		return FAILED ("failed to find value #1");

	if (!dn_ptr_array_find (array, letters [1], &i) || i != 1)
		return FAILED ("failed to find value #2");

	if (!dn_ptr_array_find (array, letters [2], &i) || i != 2)
		return FAILED ("failed to find value #3");

	if (!dn_ptr_array_find (array, letters [3], &i) || i != 3)
		return FAILED ("failed to find value #4");

	if (!dn_ptr_array_find (array, letters [4], &i) || i != 4)
		return FAILED ("failed to find value #5");

	dn_ptr_array_free (array, true);

	return OK;
}

static
RESULT
test_ptr_array_ex_alloc (void)
{
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_alloc_capacity (void)
{
	dn_ptr_array_t *array = NULL;

	array = dn_ptr_array_ex_alloc_capacity (ARRAY_SIZE (items));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (dn_ptr_array_capacity (array) < ARRAY_SIZE (items))
		return FAILED ("capacity didn't match");

	void *data = array->data;
	for (int32_t i = 0; i < ARRAY_SIZE (items); ++i) {
		dn_ptr_array_ex_push_back (array, (void *)items [i]);
		if (array->data != data)
			return FAILED ("array pre-alloc failed");
	}

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
ptr_array_free_clear_func(void *data)
{
	(*((uint32_t *)data))++;
}

static
void
DN_CALLBACK_CALLTYPE
ptr_array_free_func(void *data)
{
	free (data);
}

static
RESULT
test_ptr_array_ex_free (void)
{
	int32_t count = 0;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);

	dn_ptr_array_ex_free (&array);
	if (array != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_ptr_array_ex_foreach_free (void)
{
	int32_t count = 0;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);

	dn_ptr_array_ex_for_each_free (&array, ptr_array_free_clear_func);
	if (array != NULL)
		return FAILED ("ex_for_each_free failed");

	if (count != 2)
		return FAILED ("callback called incorrect number of times");

	return OK;
}

static
RESULT
test_ptr_array_ex_clear (void)
{
	uint32_t count = 0;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);

	dn_ptr_array_ex_clear (array);
	if (array->len != 0)
		return FAILED ("array len didn't match #2");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_foreach_clear (void)
{
	uint32_t count = 0;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);
	dn_ptr_array_ex_push_back (array, &count);

	uint32_t capacity = dn_ptr_array_capacity (array);

	dn_ptr_array_ex_for_each_clear (array, ptr_array_free_clear_func);
	if (array->len != 0)
		return FAILED ("array len didn't match #2");

	if (dn_ptr_array_capacity (array) != capacity)
		return FAILED ("incorrect array capacity");

	if (count != 5)
		return FAILED ("allback called incorrect number of times");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_foreach_it (void)
{
	uint32_t count = 0;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_ptr_array_ex_push_back (array, INT32_TO_POINTER (i));

	DN_PTR_ARRAY_EX_FOREACH_BEGIN (array, uint32_t *, value) {
		if (POINTER_TO_INT32 (value) != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_PTR_ARRAY_EX_FOREACH_END;

	if (count != dn_ptr_array_ex_size (array))
		return FAILED ("foreach iterator failed #2");

	dn_ptr_array_ex_free (&array);
	return OK;
}

static
RESULT
test_ptr_array_ex_foreach_rev_it (void)
{
	uint32_t count = 100;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_ptr_array_ex_push_back (array, INT32_TO_POINTER (i));

	DN_PTR_ARRAY_EX_FOREACH_RBEGIN (array, uint32_t *, value) {
		if (POINTER_TO_INT32 (value) != count - 1)
			return FAILED ("foreach reverse iterator failed #1");
		count--;
	} DN_PTR_ARRAY_EX_FOREACH_END;

	if (count != 0)
		return FAILED ("foreach reverse iterator failed #2");

	dn_ptr_array_ex_free (&array);
	return OK;
}

static
RESULT
test_ptr_array_ex_push_back (void)
{
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (!dn_ptr_array_ex_push_back (array, strdup ("a")))
		return FAILED ("ex_push_back failed #1");

	if (!dn_ptr_array_ex_push_back (array, strdup ("b")))
		return FAILED ("ex_push_back failed #2");

	if (!dn_ptr_array_ex_push_back (array, strdup ("c")))
		return FAILED ("ex_push_back failed #3");

	dn_ptr_array_ex_for_each_free (&array, ptr_array_free_func);
	if (array != NULL)
		return FAILED ("ex_for_each_free failed");

	return OK;
}

static
RESULT
test_ptr_array_ex_erase (void)
{
	const char *a = "a";

	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_ptr_array_ex_push_back (array, (void *)a);
	dn_ptr_array_ex_push_back (array, strdup ("b"));
	dn_ptr_array_ex_push_back (array, strdup ("c"));

	dn_ptr_array_ex_erase (array, (void *)a);

	DN_PTR_ARRAY_EX_FOREACH_BEGIN (array, const char *, value) {
		if (value == a)
			return FAILED ("ex_erase failed");
	} DN_PTR_ARRAY_EX_FOREACH_END;

	dn_ptr_array_ex_for_each_free (&array, ptr_array_free_func);
	if (array != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_ptr_array_ex_empty (void)
{
	uint32_t v = 27;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc ();
	if (dn_ptr_array_ex_size (array) != 0)
		return FAILED ("array len didn't match");

	dn_ptr_array_ex_push_back (array, &v);

	if (dn_ptr_array_ex_empty (array))
		return FAILED ("is_empty failed #1");

	dn_ptr_array_ex_clear (array);

	if (!dn_ptr_array_ex_empty (array))
		return FAILED ("is_empty failed #2");

	dn_ptr_array_ex_free (&array);

	if (!dn_ptr_array_ex_empty (array))
		return FAILED ("is_empty failed #3");

	return OK;
}

static
RESULT
test_ptr_array_ex_default_local_alloc (void)
{
	DN_PTR_ARRAY_EX_LOCAL_ALLOCATOR (allocator, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	uint32_t capacity = dn_ptr_array_ex_buffer_capacity (DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_custom ((dn_allocator_t *)&allocator);
	if (!array)
		return FAILED ("failed array custom alloc");

	for (uint32_t i = 0; i < capacity; ++i) {
		for (uint32_t j = 0; j < ARRAY_SIZE (items); ++j) {
			if (!dn_ptr_array_ex_push_back (array, (void *)items [j]))
				return FAILED ("failed array append using custom alloc");
		}
	}

	for (uint32_t i = 0; i < capacity; ++i) {
		for (uint32_t j = 0; j < ARRAY_SIZE (items); ++j) {
			if (dn_ptr_array_index (array, ARRAY_SIZE (items) * i + j) != items [j])
				return FAILED ("array realloc failure using default local alloc");
		}
	}

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
bool
ptr_array_owned_by_static_allocator (
	dn_allocator_static_t *allocator,
	dn_ptr_array_t *array)
{
	return	allocator->_data._begin <= dn_ptr_array_ex_data (array, void *) &&
		allocator->_data._end > dn_ptr_array_ex_data (array, void *);
}

static
RESULT
test_ptr_array_ex_local_alloc (void)
{
	uint8_t buffer [1024];
	dn_allocator_static_dynamic_t allocator;

	dn_allocator_static_dynamic_init (&allocator, buffer, ARRAY_SIZE (buffer));
	memset (buffer, 0, ARRAY_SIZE (buffer));

	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_custom ((dn_allocator_t *)&allocator);
	if (!array)
		return FAILED ("failed array custom alloc");

	// All should fit in static allocator.
	for (uint32_t i = 0; i < ARRAY_SIZE (items); ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i]))
			return FAILED ("failed array append using custom alloc #1");
	}

	if (!ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("custom alloc using static allocator failed");

	// Make sure we run out of static allocator memory, should switch to dynamic allocator.
	for (uint32_t i = 0; i < ARRAY_SIZE (buffer); ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc #2");
	}

	if (ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("custom alloc using dynamic allocator failed");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_local_alloc_capacity (void)
{
	uint8_t buffer [DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE];
	dn_allocator_static_dynamic_t allocator;

	dn_allocator_static_dynamic_init (&allocator, buffer, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	uint32_t capacity = DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_CAPACITY;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc");

	if (dn_ptr_array_capacity (array) != 256)
		return FAILED ("default local array should have 256 in capacity #1");

	// Make sure pre-allocted static allocator is used.
	if (!ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("custom alloc using static allocator failed #1");

	// Add pre-allocated amount of items, should fit into static buffer.
	for (uint32_t i = 0; i < capacity; ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc");
	}

	if (dn_ptr_array_capacity (array) != 256)
		return FAILED ("default local array should have 256 in capacity #2");

	// Make sure pre-allocted static allocator is used without reallocs (would cause OOM and switch to dynamic).
	if (!ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("custom alloc using static allocator failed #2");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_static_alloc_capacity (void)
{
	uint8_t buffer [DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE];
	dn_allocator_static_t allocator;

	dn_allocator_static_init (&allocator, buffer, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	uint32_t capacity = DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_CAPACITY;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc");

	// Add pre-allocated amount of items, should fit into static buffer.
	for (uint32_t i = 0; i < capacity; ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc");
	}

	// Adding one more should hit OOM.
	if (dn_ptr_array_ex_push_back (array, (void *)items [0]))
		return FAILED ("array append failed to triggered OOM");

	// Make room for on more item.
	dn_ptr_array_ex_erase (array, (void *)items [0]);

	// Adding one more should not hit OOM.
	if (!dn_ptr_array_ex_push_back (array, (void *)items [0]))
		return FAILED ("array append triggered OOM");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_static_dynamic_alloc_capacity (void)
{
	uint8_t buffer [DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE];
	dn_allocator_static_dynamic_t allocator;

	dn_allocator_static_dynamic_init (&allocator, buffer, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	uint32_t capacity = DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_CAPACITY;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc");

	// Add pre-allocated amount of items, should fit into static allocator.
	for (uint32_t i = 0; i < capacity; ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc #1");
	}

	// Make sure pre-allocted static allocator is used.
	if (!ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("custom alloc using static allocator failed");

	// Adding one more should not hit OOM.
	if (!dn_ptr_array_ex_push_back (array, (void *)items [0]))
		return FAILED ("failed array append using custom alloc #2");

	if (dn_ptr_array_capacity (array) <= capacity)
		return FAILED ("unexpected array capacity #1");

	capacity = dn_ptr_array_capacity (array);

	// Make room for on more item.
	dn_ptr_array_ex_erase (array, (void *)items [0]);

	if (dn_ptr_array_capacity (array) < capacity)
		return FAILED ("unexpected array capacity #2");

	// Validate continious use of dynamic allocator.
	if (ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("unexpected switch to static allocator");

	// Adding one more should not hit OOM.
	if (!dn_ptr_array_ex_push_back (array, (void *)items [0]))
		return FAILED ("failed array append using custom alloc #3");

	if (dn_ptr_array_capacity (array) < capacity)
		return FAILED ("unexpected array capacity #3");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_static_reset_alloc_capacity (void)
{
	uint8_t buffer [DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE];
	dn_allocator_static_t allocator;

	dn_allocator_static_init (&allocator, buffer, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	uint32_t capacity = DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_CAPACITY;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc #1");

	// Add pre-allocated amount of items, should fit into static allocator.
	for (uint32_t i = 0; i < capacity; ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc");
	}

	// Adding one more should hit OOM.
	if (dn_ptr_array_ex_push_back (array, (void *)items [0]))
		return FAILED ("array append failed to triggered OOM");

	dn_ptr_array_ex_free (&array);

	// Reset static allocator.
	dn_allocator_static_reset (&allocator);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc #2");

	// Add pre-allocated amount of items, should fit into static buffer.
	for (uint32_t i = 0; i < capacity; ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc");
	}

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_ex_static_dynamic_reset_alloc_capacity (void)
{
	uint8_t buffer [DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE];
	dn_allocator_static_dynamic_t allocator;

	dn_allocator_static_dynamic_init (&allocator, buffer, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	uint32_t capacity = DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_CAPACITY;
	dn_ptr_array_t *array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc #1");

	// Add pre-allocated amount of items, should fit into static allocator.
	for (uint32_t i = 0; i < capacity; ++i) {
		if (!dn_ptr_array_ex_push_back (array, (void *)items [i % ARRAY_SIZE (items)]))
			return FAILED ("failed array append using custom alloc #1");
	}

	// Adding one more should not hit OOM but switch to dynamic allocator.
	if (!dn_ptr_array_ex_push_back (array, (void *)items [0]))
		return FAILED ("failed array append using custom alloc #2");

	// Validate use of dynamic allocator.
	if (ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("unexpected switch to static allocator #1");

	dn_ptr_array_ex_free (&array);

	array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc #2");

	// Validate use of dynamic allocator.
	if (ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("unexpected switch to static allocator #2");

	dn_ptr_array_ex_free (&array);

	// Reset static part of allocator.
	dn_allocator_static_dynamic_reset (&allocator);
	memset (buffer, 0, DN_PTR_ARRAY_EX_DEFAULT_LOCAL_ARRAY_BUFFER_SIZE);

	array = dn_ptr_array_ex_alloc_capacity_custom ((dn_allocator_t *)&allocator, capacity);
	if (!array)
		return FAILED ("failed array custom alloc #2");

	// Validate use of static allocator.
	if (!ptr_array_owned_by_static_allocator ((dn_allocator_static_t *)&allocator, array))
		return FAILED ("custom alloc using static allocator failed");

	dn_ptr_array_ex_free (&array);

	return OK;
}

static
RESULT
test_ptr_array_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_ptr_array_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_ptr_array_memory_diff_snapshot, &dn_ptr_array_memory_start_snapshot, &dn_ptr_array_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_ptr_array_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_ptr_array_tests [] = {
	{"test_ptr_array_setup", test_ptr_array_setup},
	{"test_ptr_array_new", test_ptr_array_new},
	{"test_ptr_array_free", test_ptr_array_free},
	{"test_ptr_array_sized_new", test_ptr_array_sized_new},
	{"test_ptr_array_for_iterate", test_ptr_array_for_iterate},
	{"test_ptr_array_foreach_iterate", test_ptr_array_foreach_iterate},
	{"test_ptr_array_set_size", test_ptr_array_set_size},
	{"test_ptr_array_remove_index", test_ptr_array_remove_index},
	{"test_ptr_array_remove_index_fast", test_ptr_array_remove_index_fast},
	{"test_ptr_array_remove", test_ptr_array_remove},
	{"test_ptr_array_sort", test_ptr_array_sort},
	{"test_ptr_array_remove_fast", test_ptr_array_remove_fast},
	{"test_ptr_array_capacity", test_ptr_array_capacity},
	{"test_ptr_array_find", test_ptr_array_find},
	{"test_ptr_array_ex_alloc", test_ptr_array_ex_alloc},
	{"test_ptr_array_ex_alloc_capacity", test_ptr_array_ex_alloc_capacity},
	{"test_ptr_array_ex_free", test_ptr_array_ex_free},
	{"test_ptr_array_ex_foreach_free", test_ptr_array_ex_foreach_free},
	{"test_ptr_array_ex_clear", test_ptr_array_ex_clear},
	{"test_ptr_array_ex_foreach_clear", test_ptr_array_ex_foreach_clear},
	{"test_ptr_array_ex_foreach_it", test_ptr_array_ex_foreach_it},
	{"test_ptr_array_ex_foreach_rev_it", test_ptr_array_ex_foreach_rev_it},
	{"test_ptr_array_ex_push_back", test_ptr_array_ex_push_back},
	{"test_ptr_array_ex_erase", test_ptr_array_ex_erase},
	{"test_ptr_array_ex_empty", test_ptr_array_ex_empty},
	{"test_ptr_array_ex_default_local_alloc", test_ptr_array_ex_default_local_alloc},
	{"test_ptr_array_ex_local_alloc", test_ptr_array_ex_local_alloc},
	{"test_ptr_array_ex_local_alloc_capacity", test_ptr_array_ex_local_alloc_capacity},
	{"test_ptr_array_ex_static_alloc_capacity", test_ptr_array_ex_static_alloc_capacity},
	{"test_ptr_array_ex_static_dynamic_alloc_capacity", test_ptr_array_ex_static_dynamic_alloc_capacity},
	{"test_ptr_array_ex_static_reset_alloc_capacity", test_ptr_array_ex_static_reset_alloc_capacity},
	{"test_ptr_array_ex_static_dynamic_reset_alloc_capacity", test_ptr_array_ex_static_dynamic_reset_alloc_capacity},
	{"test_ptr_array_teardown", test_ptr_array_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_ptr_array_tests_init, dn_ptr_array_tests)
