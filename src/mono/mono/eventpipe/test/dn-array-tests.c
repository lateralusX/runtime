#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-array-ex.h>

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_array_memory_start_snapshot;
static _CrtMemState dn_array_memory_end_snapshot;
static _CrtMemState dn_array_memory_diff_snapshot;
#endif

static
RESULT
test_array_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_array_memory_start_snapshot);
#endif
	return OK;
}

static
RESULT
test_array_new (void)
{
	int32_t v = 27;
	dn_array_t *array = NULL;

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_free (array, true);

	array = dn_array_new (true, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_append_val (array, v);

	if (0 != dn_array_index (array, int32_t, 1))
		return FAILED ("zero_terminated didn't append a zero element");

	dn_array_free (array, true);

	array = dn_array_new (false, true, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (0 != dn_array_index (array, int32_t, 0))
		return FAILED ("array was not cleared when allocated");

	dn_array_free (array, true);

	return OK;
}

#define PREALLOC_SIZE 127

static
RESULT
test_array_free (void)
{
	int32_t v = 27;
	dn_array_t *array = NULL;

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_free (array, true);

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_append_val (array, v);

	int32_t *data = (int32_t *)dn_array_free (array, false);
	if (data[0] != 27)
		return FAILED ("free didn't preserve segment");

	dn_array_free_segment ((char *)data);

	return OK;
}

static
RESULT
test_array_sized_new (void)
{
	int32_t v = 27;
	dn_array_t *array = NULL;

	array = dn_array_sized_new (false, false, sizeof (int32_t), PREALLOC_SIZE);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		dn_array_t * array2 = dn_array_insert_val (array, i, i);
		if (array->data != array2->data)
			return FAILED ("array pre-alloc failed");
	}

	dn_array_free (array, true);

	array = dn_array_sized_new (true, false, sizeof (int32_t), PREALLOC_SIZE);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_append_val (array, v);

	if (0 != dn_array_index (array, int32_t, 1))
		return FAILED ("zero_terminated didn't append a zero element");

	dn_array_free (array, true);

	array = dn_array_sized_new (false, true, sizeof (int32_t), PREALLOC_SIZE);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		if (0 != dn_array_index (array, int32_t, i))
			return FAILED ("array was not cleared when allocated");
	}

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_index (void)
{
	int32_t v;
	dn_array_t *array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	v = 27;
	dn_array_append_val (array, v);

	if (27 != dn_array_index (array, int32_t, 0))
		return FAILED ("dn_array_index failed");

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_append (void)
{
	int32_t v;
	dn_array_t *array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (0 != array->len)
		return FAILED ("initial array length not zero");

	v = 27;

	dn_array_append_val (array, v);

	if (1 != array->len)
		return FAILED ("array append failed");

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_append_vals (void)
{
	int32_t v = 0;
	int32_t vs [] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	dn_array_t *array = NULL;

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_append_val (array, v);
	dn_array_append_vals (array, vs, ARRAY_SIZE (vs));

	for (int32_t i = 0; i < array->len; ++i) {
		if (i != dn_array_index (array, int32_t, i))
			return FAILED ("append vals failed");
	}

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_insert_val (void)
{
	void *ptr0, *ptr1, *ptr2, *ptr3;
	dn_array_t *array = dn_array_new (false, false, sizeof (void *));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_insert_val (array, 0, array);

	if (array != dn_array_index (array, void *, 0))
		return FAILED ("1 The value in the array is incorrect");

	dn_array_insert_val (array, 1, array);
	if (array != dn_array_index (array, void *, 1))
		return FAILED ("2 The value in the array is incorrect");

	dn_array_insert_val (array, 2, array);
	if (array != dn_array_index (array, void *, 2))
		return FAILED ("3 The value in the array is incorrect");

	dn_array_free (array, true);
	array = dn_array_new (false, false, sizeof (void *));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	ptr0 = array;
	ptr1 = array + 1;
	ptr2 = array + 2;
	ptr3 = array + 3;

	dn_array_insert_val (array, 0, ptr0);
	dn_array_insert_val (array, 1, ptr1);
	dn_array_insert_val (array, 2, ptr2);
	dn_array_insert_val (array, 1, ptr3);
	if (ptr0 != dn_array_index (array, void *, 0))
		return FAILED ("4 The value in the array is incorrect");
	if (ptr3 != dn_array_index (array, void *, 1))
		return FAILED ("5 The value in the array is incorrect");
	if (ptr1 != dn_array_index (array, void *, 2))
		return FAILED ("6 The value in the array is incorrect");
	if (ptr2 != dn_array_index (array, void *, 3))
		return FAILED ("7 The value in the array is incorrect");

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_insert_vals (void)
{
	int32_t vs1 [] = {0, 1, 2};
	int32_t vs2 [] = { 3 };
	int32_t vs3 [] = { 4, 5, 6 };
	int32_t vs4 [] = { 7, 8, 9 };

	dn_array_t *array = NULL;

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_insert_vals (array, 0, vs1, ARRAY_SIZE (vs1));
	dn_array_insert_vals (array, 3, vs3, ARRAY_SIZE (vs3));
	dn_array_insert_vals (array, 6, vs4, ARRAY_SIZE (vs4));
	dn_array_insert_vals (array, 3, vs2, ARRAY_SIZE (vs2));

	for (int32_t i = 0; i < array->len; ++i) {
		if (i != dn_array_index (array, int32_t, i))
			return FAILED ("insert vals failed");
	}

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_remove (void)
{
	int32_t v [] = { 30, 29, 28, 27, 26, 25 };
	dn_array_t *array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_append_vals (array, v, ARRAY_SIZE (v));

	if (ARRAY_SIZE (v) != array->len)
		return FAILED ("append_vals fail");

	dn_array_remove_index (array, 3);

	if (5 != array->len)
		return FAILED ("remove_index failed to update length");

	if (26 != dn_array_index (array, int32_t, 3))
		return FAILED ("remove_index failed to update the array");

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_set_size (void)
{
	dn_array_t *array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	for (int32_t i = 0; i < 10; ++i)
		array = dn_array_insert_val (array, i, i);

	dn_array_set_size (array, 5);
	if (array->len != 5)
		return FAILED ("array len didn't match #2");

	for (int32_t i = 0; i < array->len; ++i) {
		if (i != dn_array_index (array, int32_t, i))
			return FAILED ("set size didn't preserve data");
	}

	dn_array_set_size (array, 0);
	if (array->len != 0)
		return FAILED ("array len didn't match #3");

	dn_array_free (array, true);

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match #4");

	dn_array_set_size (array, PREALLOC_SIZE);

	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		dn_array_t * array2 = dn_array_insert_val (array, i, i);
		if (array->data != array2->data)
			return FAILED ("array set size failed");
	}

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_append_zero_terminated (void)
{
	int32_t v;
	dn_array_t *array = dn_array_new (true, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	v = 27;
	dn_array_append_val (array, v);

	if (27 != dn_array_index (array, int32_t, 0))
		return FAILED ("dn_array_append_val failed");

	if (0 != dn_array_index (array, int32_t, 1))
		return FAILED ("zero_terminated didn't append a zero element");

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_big (void)
{
	dn_array_t *array;
	int32_t i;

	array = dn_array_new (false, false, sizeof (int32_t));
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (i = 0; i < 10000; i++)
		dn_array_append_val (array, i);

	for (i = 0; i < 10000; i++)
		if (dn_array_index (array, int32_t, i) != i)
			return FAILED ("array value didn't match");

	dn_array_free (array, true);

	return OK;
}

static
RESULT
test_array_ex_alloc (void)
{
	dn_array_t *array = dn_array_ex_alloc (int32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_ex_free (&array);

	return OK;
}

static
RESULT
test_array_ex_alloc_capacity (void)
{
	int32_t v = 27;
	dn_array_t *array = dn_array_ex_alloc_capacity (int32_t, 1024);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	uint32_t capacity = dn_array_capacity (array);
	if (capacity < 1024)
		return FAILED ("ex_alloc_capacity failed #1");

	char * pre_alloc_data = array->data;
	for (uint32_t i = 0; i < 1024; ++i) {
		dn_array_ex_push_back (array, v);
		if (pre_alloc_data != array->data || capacity != dn_array_capacity (array))
			return FAILED ("ex_alloc_capacity failed #2");
	}

	dn_array_ex_free (&array);
	if (array != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_array_ex_free (void)
{
	int32_t v = 27;
	dn_array_t *array = dn_array_ex_alloc (int32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_array_ex_push_back (array, v);
	dn_array_ex_push_back (array, v);

	dn_array_ex_free (&array);
	if (array != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_array_ex_foreach_it (void)
{
	uint32_t count = 0;
	dn_array_t *array = dn_array_ex_alloc (uint32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_array_ex_push_back (array, i);

	DN_ARRAY_EX_FOREACH_BEGIN (array, uint32_t, value) {
		if (value != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_ARRAY_EX_FOREACH_END;

	if (count != dn_array_ex_size (array))
		return FAILED ("foreach iterator failed #2");

	dn_array_ex_free (&array);
	return OK;
}

static
RESULT
test_array_ex_foreach_rev_it (void)
{
	uint32_t count = 100;
	dn_array_t *array = dn_array_ex_alloc (uint32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_array_ex_push_back (array, i);

	DN_ARRAY_EX_FOREACH_RBEGIN (array, uint32_t, value) {
		if (value != count - 1)
			return FAILED ("foreach reverse iterator failed #1");
		count--;
	} DN_ARRAY_EX_FOREACH_END;

	if (count != 0)
		return FAILED ("foreach reverse iterator failed #2");

	dn_array_ex_free (&array);
	return OK;
}

static
RESULT
test_array_ex_push_back (void)
{
	dn_array_t *array = dn_array_ex_alloc (int32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_array_ex_push_back (array, i);

	for (int32_t i = 0; i < array->len; ++i) {
		if (i != dn_array_index (array, int32_t, i))
			return FAILED ("array append failed");
	}

	dn_array_ex_free (&array);

	return OK;
}

static
RESULT
test_array_ex_erase (void)
{
	int32_t vs [] = { 0, 2, 3, 4, 5, 6, 7, 9 };
	dn_array_t *array = dn_array_ex_alloc (int32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_array_ex_push_back (array, i);

	dn_array_ex_erase (array, 1);
	dn_array_ex_erase (array, 7);

	for (int32_t i = 0; i < array->len; ++i) {
		if (vs [i] != dn_array_index (array, int32_t, i))
			return FAILED ("array remove failed");
	}

	dn_array_ex_free (&array);

	return OK;
}

static
RESULT
test_array_ex_clear (void)
{
	dn_array_t *array = dn_array_ex_alloc (int32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	for (int32_t i = 0; i < 10; ++i)
		dn_array_ex_push_back (array, i);

	dn_array_ex_clear (array);
	if (array->len != 0)
		return FAILED ("array len didn't match #2");

	dn_array_ex_free (&array);

	return OK;
}

static
RESULT
test_array_ex_empty (void)
{
	dn_array_t *array = dn_array_ex_alloc (int32_t);
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	for (int32_t i = 0; i < 10; ++i)
		dn_array_ex_push_back (array, i);

	if (dn_array_ex_empty (array))
		return FAILED ("is_empty failed #1");

	dn_array_ex_clear (array);

	if (!dn_array_ex_empty (array))
		return FAILED ("is_empty failed #2");

	dn_array_ex_free (&array);

	if (!dn_array_ex_empty (array))
		return FAILED ("is_empty failed #3");

	return OK;
}

static
RESULT
test_array_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_array_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_array_memory_diff_snapshot, &dn_array_memory_start_snapshot, &dn_array_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_array_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_array_tests [] = {
	{"test_array_setup", test_array_setup},
	{"test_array_new", test_array_new},
	{"test_array_sized_new", test_array_sized_new},
	{"test_array_free", test_array_free},
	{"test_array_append", test_array_append},
	{"test_array_append_vals", test_array_append_vals},
	{"test_array_insert_val", test_array_insert_val},
	{"test_array_insert_vals", test_array_insert_vals},
	{"test_array_index", test_array_index},
	{"test_array_remove", test_array_remove},
	{"test_array_set_size", test_array_set_size},
	{"test_array_append_zero_terminated", test_array_append_zero_terminated},
	{"test_array_big", test_array_big},
	{"test_array_ex_alloc", test_array_ex_alloc},
	{"test_array_ex_alloc_capacity", test_array_ex_alloc_capacity},
	{"test_array_ex_free", test_array_ex_free},
	{"test_array_ex_foreach_it", test_array_ex_foreach_it},
	{"test_array_ex_foreach_rev_it", test_array_ex_foreach_rev_it},
	{"test_array_ex_push_back", test_array_ex_push_back},
	{"test_array_ex_erase", test_array_ex_erase},
	{"test_array_ex_clear", test_array_ex_clear},
	{"test_array_ex_empty", test_array_ex_empty},
	{"test_array_teardown", test_array_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_array_tests_init, dn_array_tests)
