#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-vector.h>

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_vector_memory_start_snapshot;
static _CrtMemState dn_vector_memory_end_snapshot;
static _CrtMemState dn_vector_memory_diff_snapshot;
#endif

static
RESULT
test_vector_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_vector_memory_start_snapshot);
#endif
	return OK;
}

static
RESULT
test_vector_alloc (void)
{
	dn_vector_t *vector = NULL;

	vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_free (vector);

	vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	if (0 != dn_vector_index_t (vector, int32_t, 0))
		return FAILED ("vector was not cleared when allocated");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_init (void)
{
	dn_vector_t vector;

	if (!dn_vector_init (&vector, sizeof (dn_vector_t), sizeof (int32_t)))
		return FAILED ("init vector");
	
	if (vector.size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_fini (&vector);

	if (!dn_vector_init_t (&vector, int32_t))
		return FAILED ("init vector");

	if (vector.size != 0)
		return FAILED ("vector size didn't match");

	if (0 != dn_vector_index_t (&vector, int32_t, 0))
		return FAILED ("vector was not cleared when allocated");

	dn_vector_fini (&vector);

	return OK;
}

#define PREALLOC_SIZE 127

static
RESULT
test_vector_alloc_capacity (void)
{
	dn_vector_t *vector = NULL;

	vector = dn_vector_alloc_capacity (sizeof (int32_t), PREALLOC_SIZE);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	uint8_t *pre_alloc_data = vector->data;
	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		dn_vector_insert (vector, i, i);
		if (pre_alloc_data != vector->data)
			return FAILED ("vector pre-alloc failed");
	}

	dn_vector_free (vector);

	vector = dn_vector_alloc_capacity_t (int32_t, PREALLOC_SIZE);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		if (0 != dn_vector_index_t (vector, int32_t, i))
			return FAILED ("vector was not cleared when allocated");
	}

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_init_capacity (void)
{
	dn_vector_t vector;

	if (!dn_vector_init_capacity (&vector, sizeof (dn_vector_t), sizeof (int32_t), PREALLOC_SIZE))
		return FAILED ("init vector");

	if (vector.size != 0)
		return FAILED ("vector size didn't match");

	uint8_t *pre_alloc_data = vector.data;
	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		dn_vector_insert (&vector, i, i);
		if (pre_alloc_data != vector.data)
			return FAILED ("vector pre-alloc failed");
	}

	dn_vector_fini (&vector);

	if (!dn_vector_init_capacity_t (&vector, int32_t, PREALLOC_SIZE))
		return FAILED ("init vector");

	if (vector.size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		if (0 != dn_vector_index_t (&vector, int32_t, i))
			return FAILED ("vector was not cleared when allocated");
	}

	dn_vector_fini (&vector);

	return OK;
}

static
RESULT
test_vector_free (void)
{
	dn_vector_t *vector = NULL;

	vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_fini (void)
{
	dn_vector_t vector;

	if (!dn_vector_init_t (&vector, int32_t))
		return FAILED ("init vector");
	if (vector.size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_fini (&vector);

	return OK;
}

static
RESULT
test_vector_index (void)
{
	int32_t v;
	dn_vector_t *vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	v = 27;
	dn_vector_push_back (vector, v);

	if (27 != *(int32_t *)(dn_vector_index (vector, 0)))
		return FAILED ("dn_vector_index failed");

	if (27 != dn_vector_index_t (vector, int32_t, 0))
		return FAILED ("dn_vector_index failed");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_front (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (0 != *(int32_t *)(dn_vector_front (vector)))
		return FAILED ("dn_vector_front failed");

	if (0 != dn_vector_front_t (vector, int32_t))
		return FAILED ("dn_vector_front_t failed");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_back (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (9 != *(int32_t *)(dn_vector_back (vector)))
		return FAILED ("dn_vector_back failed");

	if (9 != dn_vector_back_t (vector, int32_t))
		return FAILED ("dn_vector_back_t failed");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_data (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	int32_t *vector_data = (int32_t *)(dn_vector_data (vector));
	vector_data++;

	if (1 != *vector_data)
		return FAILED ("dn_vector_data failed");

	vector_data = dn_vector_data_t (vector, int32_t);
	vector_data += 2;

	if (2 != *vector_data)
		return FAILED ("dn_vector_data_t failed");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_begin (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (dn_vector_begin (vector) != 0)
		return FAILED ("vector_begin didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (dn_vector_begin (vector) != 0)
		return FAILED ("vector_begin didn't match");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_end (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (dn_vector_end (vector) != 0)
		return FAILED ("vector_end didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (dn_vector_end (vector) != dn_vector_size (vector))
		return FAILED ("vector_end didn't didn't return expected value");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_empty (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (dn_vector_empty (vector))
		return FAILED ("empty failed #1");

	dn_vector_clear (vector);

	if (!dn_vector_empty (vector))
		return FAILED ("empty failed #2");

	dn_vector_free (vector);
	vector = NULL;

	if (!dn_vector_empty (vector))
		return FAILED ("empty failed #3");

	return OK;
}

static
RESULT
test_vector_size (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != dn_vector_size (vector))
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (dn_vector_size (vector) != 10)
		return FAILED ("vector_size failed #1");

	dn_vector_clear (vector);

	if (dn_vector_size (vector) != 0)
		return FAILED ("vector_size failed #2");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_max_size (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (dn_vector_max_size (vector) != UINT32_MAX)
		return FAILED ("vector max size didn't match");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_reserve (void)
{
	dn_vector_t *vector = dn_vector_alloc_capacity_t (int32_t, 10);
	uint32_t capacity = dn_vector_capacity (vector);

	dn_vector_reserve (vector, 1024);
	if (capacity >= dn_vector_capacity (vector))
		return FAILED ("vector reserve failed #1");

	if (dn_vector_capacity (vector) < 1024)
		return FAILED ("vector reserve failed #2");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_capacity (void)
{
	dn_vector_t *vector = dn_vector_alloc_capacity_t (int32_t, 1024);
	uint32_t capacity = dn_vector_capacity (vector);
	if (capacity < 1024)
		return FAILED ("vector capacity didn't match #1");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (dn_vector_capacity (vector) != capacity)
		return FAILED ("vector capacity didn't match #2");

	dn_vector_clear (vector);

	if (dn_vector_capacity (vector) != capacity)
		return FAILED ("vector capacity didn't match #3");

	dn_vector_free (vector);

	vector = dn_vector_alloc_capacity_t (int32_t, 0);
	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_insert_range (void)
{
	int32_t vs1 [] = {0, 1, 2};
	int32_t vs2 [] = { 3 };
	int32_t vs3 [] = { 4, 5, 6 };
	int32_t vs4 [] = { 7, 8, 9 };

	dn_vector_t *vector = NULL;

	vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_insert_range (vector, 0, (const uint8_t *)vs1, ARRAY_SIZE (vs1));
	dn_vector_insert_range (vector, 3, (const uint8_t *)vs3, ARRAY_SIZE (vs3));
	dn_vector_insert_range (vector, 6, (const uint8_t *)vs4, ARRAY_SIZE (vs4));
	dn_vector_insert_range (vector, 3, (const uint8_t *)vs2, ARRAY_SIZE (vs2));

	for (uint32_t i = 0; i < vector->size; ++i) {
		if (i != dn_vector_index_t (vector, int32_t, i))
			return FAILED ("insert failed");
	}

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_insert_range_end (void)
{
	int32_t v = 0;
	int32_t vs [] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	dn_vector_t *vector = NULL;

	vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_push_back (vector, v);
	dn_vector_insert_range (vector, dn_vector_end (vector), (const uint8_t *)vs, ARRAY_SIZE (vs));

	for (uint32_t i = 0; i < vector->size; ++i) {
		if (i != dn_vector_index_t (vector, int32_t, i))
			return FAILED ("insert_range failed");
	}

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_insert (void)
{
	void *ptr0, *ptr1, *ptr2, *ptr3;
	dn_vector_t *vector = dn_vector_alloc (sizeof (void *));
	if (vector->size != 0)
		return FAILED ("1 Vector size didn't match");

	dn_vector_insert (vector, 0, vector);

	if (vector != dn_vector_index_t (vector, void *, 0))
		return FAILED ("2 The value in the vector is incorrect");

	dn_vector_insert (vector, 1, vector);
	if (vector != dn_vector_index_t (vector, void *, 1))
		return FAILED ("3 The value in the vector is incorrect");

	dn_vector_insert (vector, 2, vector);
	if (vector != dn_vector_index_t (vector, void *, 2))
		return FAILED ("4 The value in the vector is incorrect");

	dn_vector_free (vector);

	vector = dn_vector_alloc (sizeof (void *));
	if (vector->size != 0)
		return FAILED ("5 Vector size didn't match");

	ptr0 = vector;
	ptr1 = vector + 1;
	ptr2 = vector + 2;
	ptr3 = vector + 3;

	dn_vector_insert (vector, 0, ptr0);
	dn_vector_insert (vector, 1, ptr1);
	dn_vector_insert (vector, 2, ptr2);
	dn_vector_insert (vector, 1, ptr3);
	if (ptr0 != dn_vector_index_t (vector, void *, 0))
		return FAILED ("6 The value in the vector is incorrect");
	if (ptr3 != dn_vector_index_t (vector, void *, 1))
		return FAILED ("7 The value in the vector is incorrect");
	if (ptr1 != dn_vector_index_t (vector, void *, 2))
		return FAILED ("8 The value in the vector is incorrect");
	if (ptr2 != dn_vector_index_t (vector, void *, 3))
		return FAILED ("9 The value in the vector is incorrect");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_push_back (void)
{
	int32_t v;
	dn_vector_t *vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	if (0 != vector->size)
		return FAILED ("initial vector size is not zero");

	v = 27;

	dn_vector_push_back (vector, v);

	if (1 != vector->size)
		return FAILED ("vector push_back failed");

	if (27 != dn_vector_index_t (vector, int32_t, 0))
		return FAILED ("dn_vector_index failed");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_push_back_2 (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	for (uint32_t i = 0; i < vector->size; ++i) {
		if (i != dn_vector_index_t (vector, int32_t, i))
			return FAILED ("vector push_back failed");
	}

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_pop_back (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	if (dn_vector_back_t (vector, int32_t) != 9)
		return FAILED ("vector back failed");

	dn_vector_pop_back (vector);

	if (dn_vector_back_t (vector, int32_t) != 8)
		return FAILED ("vector pop_back failed");

	dn_vector_pop_back (vector);

	if (dn_vector_back_t (vector, int32_t) != 7)
		return FAILED ("vector pop_back failed");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_erase (void)
{
	int32_t v [] = { 30, 29, 28, 27, 26, 25 };
	dn_vector_t *vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	dn_vector_insert_range (vector, dn_vector_end (vector), (const uint8_t *)v, ARRAY_SIZE (v));

	if (ARRAY_SIZE (v) != vector->size)
		return FAILED ("insert_range fail");

	dn_vector_erase (vector, 3);

	if (5 != vector->size)
		return FAILED ("erase failed to update size");

	if (26 != dn_vector_index_t (vector, int32_t, 3))
		return FAILED ("erase failed to update the vector");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_erase_2 (void)
{
	int32_t vs [] = { 0, 2, 3, 4, 5, 6, 7, 9 };
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	dn_vector_erase (vector, 1);
	dn_vector_erase (vector, 7);

	for (uint32_t i = 0; i < vector->size; ++i) {
		if (vs [i] != dn_vector_index_t (vector, int32_t, i))
			return FAILED ("vector erase failed");
	}

	dn_vector_free (vector);

	return OK;
}

//erase_fast

static
RESULT
test_vector_resize (void)
{
	dn_vector_t *vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match #1");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_insert (vector, i, i);

	dn_vector_resize (vector, 5);
	if (vector->size != 5)
		return FAILED ("vector size didn't match #2");

	for (uint32_t i = 0; i < vector->size; ++i) {
		if (i != dn_vector_index_t (vector, int32_t, i))
			return FAILED ("resize didn't preserve data");
	}

	dn_vector_resize (vector, 0);
	if (vector->size != 0)
		return FAILED ("vector size didn't match #3");

	dn_vector_free (vector);

	vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match #4");

	dn_vector_resize (vector, PREALLOC_SIZE);

	const uint8_t *vector_data = vector->data;
	for (int32_t i = 0; i < PREALLOC_SIZE; ++i) {
		dn_vector_insert (vector, i, i);
		if (vector->data != vector_data)
			return FAILED ("vector resize failed");
	}

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_clear (void)
{
	dn_vector_t *vector = dn_vector_alloc_t (int32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match #1");

	for (int32_t i = 0; i < 10; ++i)
		dn_vector_push_back (vector, i);

	dn_vector_clear (vector);
	if (vector->size != 0)
		return FAILED ("vector size didn't match #2");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_foreach_it (void)
{
	uint32_t count = 0;
	dn_vector_t *vector = dn_vector_alloc_t (uint32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_vector_push_back (vector, i);

	DN_VECTOR_FOREACH_BEGIN (vector, uint32_t, value) {
		if (value != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_VECTOR_FOREACH_END;

	if (count != dn_vector_size (vector))
		return FAILED ("foreach iterator failed #2");

	dn_vector_free (vector);
	return OK;
}

static
RESULT
test_vector_foreach_rev_it (void)
{
	uint32_t count = 100;
	dn_vector_t *vector = dn_vector_alloc_t (uint32_t);
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (uint32_t i = 0; i < 100; ++i)
		dn_vector_push_back (vector, i);

	DN_VECTOR_FOREACH_RBEGIN (vector, uint32_t, value) {
		if (value != count - 1)
			return FAILED ("foreach reverse iterator failed #1");
		count--;
	} DN_VECTOR_FOREACH_END;

	if (count != 0)
		return FAILED ("foreach reverse iterator failed #2");

	dn_vector_free (vector);
	return OK;
}

static
RESULT
test_vector_big (void)
{
	dn_vector_t *vector;
	int32_t i;

	vector = dn_vector_alloc (sizeof (int32_t));
	if (vector->size != 0)
		return FAILED ("vector size didn't match");

	for (i = 0; i < 10000; i++)
		dn_vector_push_back (vector, i);

	for (i = 0; i < 10000; i++)
		if (dn_vector_index_t (vector, int32_t, i) != i)
			return FAILED ("vector value didn't match");

	dn_vector_free (vector);

	return OK;
}

static
RESULT
test_vector_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_vector_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_vector_memory_diff_snapshot, &dn_vector_memory_start_snapshot, &dn_vector_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_vector_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_vector_tests [] = {
	{"test_vector_setup", test_vector_setup},
	{"test_vector_alloc", test_vector_alloc},
	{"test_vector_init", test_vector_init},
	{"test_vector_alloc_capacity", test_vector_alloc_capacity},
	{"test_vector_init_capacity", test_vector_init_capacity},
	{"test_vector_free", test_vector_free},
	{"test_vector_fini", test_vector_fini},
	{"test_vector_index", test_vector_index},
	{"test_vector_front", test_vector_front},
	{"test_vector_back", test_vector_back},
	{"test_vector_data", test_vector_data},
	{"test_vector_begin", test_vector_begin},
	{"test_vector_end", test_vector_end},
	{"test_vector_empty", test_vector_empty},
	{"test_vector_size", test_vector_size},
	{"test_vector_max_size", test_vector_max_size},
	{"test_vector_reserve", test_vector_reserve},
	{"test_vector_capacity", test_vector_capacity},
	{"test_vector_insert_range", test_vector_insert_range},
	{"test_vector_insert_range_end", test_vector_insert_range_end},
	{"test_vector_insert", test_vector_insert},
	{"test_vector_push_back", test_vector_push_back},
	{"test_vector_push_back_2", test_vector_push_back_2},
	{"test_vector_pop_back", test_vector_pop_back},
	{"test_vector_erase", test_vector_erase},
	{"test_vector_erase_2", test_vector_erase_2},
	{"test_vector_resize", test_vector_resize},
	{"test_vector_clear", test_vector_clear},
	{"test_vector_foreach_it", test_vector_foreach_it},
	{"test_vector_foreach_rev_it", test_vector_foreach_rev_it},
	{"test_vector_big", test_vector_big},
	{"test_vector_teardown", test_vector_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_vector_tests_init, dn_vector_tests)
