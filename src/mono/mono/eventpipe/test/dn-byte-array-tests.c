#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-byte-array-ex.h>

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_byte_array_memory_start_snapshot;
static _CrtMemState dn_byte_array_memory_end_snapshot;
static _CrtMemState dn_byte_array_memory_diff_snapshot;
#endif

static
RESULT
test_byte_array_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_byte_array_memory_start_snapshot);
#endif
	return OK;
}

static
RESULT
test_byte_array_new (void)
{
	dn_byte_array_t *array = dn_byte_array_new ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	if (array->data [0] != 0)
		return FAILED ("byte array new failed");

	dn_byte_array_free (array, true);

	return OK;
}

static
RESULT
test_byte_array_free (void)
{
	dn_byte_array_t *array = dn_byte_array_new ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	uint8_t *data = dn_byte_array_free (array, false);

	if (data [0] != 0)
		return FAILED ("byte array free failed");

	dn_byte_array_free_segment (data);

	return OK;
}

static
RESULT
test_byte_array_append (void)
{
	uint8_t vs [] = { 1, 2, 3 };
	dn_byte_array_t *array = dn_byte_array_new ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	dn_byte_array_append (array, vs, ARRAY_SIZE (vs));
	dn_byte_array_append (array, vs, ARRAY_SIZE (vs));
	dn_byte_array_append (array, vs, ARRAY_SIZE (vs));

	if (array->len != 9)
		return FAILED ("array len didn't match #2");

	if (array->data [1] != 2 || array->data [3] != 1 || array->data [5] != 3)
		return FAILED ("array append failed");

	dn_byte_array_free (array, true);

	return OK;
}

static
RESULT
test_byte_array_set_size (void)
{
	dn_byte_array_t *array = dn_byte_array_new ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	dn_byte_array_set_size (array, 10);
	if (array->len != 10)
		return FAILED ("array len didn't match #2");

	for (int32_t i = 0; i < array->len; ++i)
		array->data [i] = i;

	dn_byte_array_set_size (array, 5);
	if (array->len != 5)
		return FAILED ("array len didn't match #3");

	for (int32_t i = 0; i < array->len; ++i) {
		if (array->data [i] != i)
			return FAILED ("set size didn't preserve data");
	}

	dn_byte_array_set_size (array, 0);
	if (array->len != 0)
		return FAILED ("array len didn't match #4");

	dn_byte_array_free (array, true);

	return OK;
}

static
RESULT
test_byte_array_ex_alloc (void)
{
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_byte_array_ex_free (&array);

	return OK;
}

static
RESULT
test_byte_array_ex_alloc_capacity (void)
{
	uint8_t v = 0xFF;
	dn_byte_array_t *array = dn_byte_array_ex_alloc_capacity (1024);
	if (array->len != 0)
		return FAILED ("array len didn't match");

	uint32_t capacity = dn_byte_array_capacity (array);
	if (capacity < 1024)
		return FAILED ("ex_alloc_capacity failed #1");

	uint8_t *pre_alloc_data = array->data;
	for (uint32_t i = 0; i < 1024; ++i) {
		dn_byte_array_append (array, &v, 1);
		if (pre_alloc_data != array->data || capacity != dn_byte_array_capacity (array))
			return FAILED ("ex_alloc_capacity failed #2");
	}

	dn_byte_array_ex_free (&array);
	if (array != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_byte_array_ex_free (void)
{
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	dn_byte_array_ex_free (&array);
	if (array != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_byte_array_ex_foreach_it (void)
{
	uint8_t count = 0;
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint8_t i = 0; i < 10; ++i)
		dn_byte_array_ex_push_back (array, i);

	DN_BYTE_ARRAY_EX_FOREACH_BEGIN (array, uint8_t, value) {
		if (value != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_BYTE_ARRAY_EX_FOREACH_END;

	if (count != (uint8_t)dn_byte_array_ex_size (array))
		return FAILED ("foreach iterator failed #2");

	dn_byte_array_ex_free (&array);
	return OK;
}

static
RESULT
test_byte_array_ex_foreach_rev_it (void)
{
	uint8_t count = 10;
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint8_t i = 0; i < 10; ++i)
		dn_byte_array_ex_push_back (array, i);

	DN_BYTE_ARRAY_EX_FOREACH_RBEGIN (array, uint8_t, value) {
		if (value != count - 1)
			return FAILED ("foreach reverse iterator failed #1");
		count--;
	} DN_BYTE_ARRAY_EX_FOREACH_END;

	if (count != 0)
		return FAILED ("foreach reverse iterator failed #2");

	dn_byte_array_ex_free (&array);
	return OK;
}

static
RESULT
test_byte_array_ex_push_back (void)
{
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint8_t i = 0; i < 10; ++i)
		dn_byte_array_ex_push_back (array, i);

	for (uint8_t i = 0; i < array->len; ++i) {
		if (i != dn_byte_array_ex_data (array) [i])
			return FAILED ("array append failed");
	}

	dn_byte_array_ex_free (&array);

	return OK;
}

static
RESULT
test_byte_array_ex_erase (void)
{
	uint8_t vs [] = { 0, 2, 3, 4, 5, 6, 7, 9 };
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match");

	for (uint8_t i = 0; i < 10; ++i)
		dn_byte_array_ex_push_back (array, i);

	dn_byte_array_ex_erase (array, 1);
	dn_byte_array_ex_erase (array, 7);

	for (uint8_t i = 0; i < array->len; ++i) {
		if (vs [i] != dn_byte_array_ex_data (array) [i])
			return FAILED ("array remove failed");
	}

	dn_byte_array_ex_free (&array);

	return OK;
}

static
RESULT
test_byte_array_ex_clear (void)
{
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	for (uint8_t i = 0; i < 10; ++i)
		dn_byte_array_ex_push_back (array, i);

	dn_byte_array_ex_clear (array);
	if (array->len != 0)
		return FAILED ("array len didn't match #2");

	dn_byte_array_ex_free (&array);

	return OK;
}

static
RESULT
test_byte_array_ex_empty (void)
{
	dn_byte_array_t *array = dn_byte_array_ex_alloc ();
	if (array->len != 0)
		return FAILED ("array len didn't match #1");

	for (uint8_t i = 0; i < 10; ++i)
		dn_byte_array_ex_push_back (array, i);

	if (dn_byte_array_ex_empty (array))
		return FAILED ("is_empty failed #1");

	dn_byte_array_ex_clear (array);

	if (!dn_byte_array_ex_empty (array))
		return FAILED ("is_empty failed #2");

	dn_byte_array_ex_free (&array);

	if (!dn_byte_array_ex_empty (array))
		return FAILED ("is_empty failed #3");

	return OK;
}

static
RESULT
test_byte_array_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_byte_array_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_byte_array_memory_diff_snapshot, &dn_byte_array_memory_start_snapshot, &dn_byte_array_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_byte_array_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_byte_array_tests [] = {
	{"test_byte_array_setup", test_byte_array_setup},
	{"test_byte_array_new", test_byte_array_new},
	{"test_byte_array_free", test_byte_array_free},
	{"test_byte_array_append", test_byte_array_append},
	{"test_byte_array_set_size", test_byte_array_set_size},
	{"test_byte_array_ex_alloc", test_byte_array_ex_alloc},
	{"test_byte_array_ex_alloc_capacity", test_byte_array_ex_alloc_capacity},
	{"test_byte_array_ex_free", test_byte_array_ex_free},
	{"test_byte_array_ex_foreach_it", test_byte_array_ex_foreach_it},
	{"test_byte_array_ex_foreach_rev_it", test_byte_array_ex_foreach_rev_it},
	{"test_byte_array_ex_push_back", test_byte_array_ex_push_back},
	{"test_byte_array_ex_erase", test_byte_array_ex_erase},
	{"test_byte_array_ex_clear", test_byte_array_ex_clear},
	{"test_byte_array_ex_empty", test_byte_array_ex_empty},
	{"test_byte_array_teardown", test_byte_array_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_byte_array_tests_init, dn_byte_array_tests)
