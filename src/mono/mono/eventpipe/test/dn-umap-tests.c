#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-umap.h>


#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_umap_memory_start_snapshot;
static _CrtMemState dn_umap_memory_end_snapshot;
static _CrtMemState dn_umap_memory_diff_snapshot;
#endif

#define POINTER_TO_INT32(v) ((int32_t)(ptrdiff_t)(v))
#define INT32_TO_POINTER(v) ((void *)(ptrdiff_t)(v))

static
RESULT
test_umap_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_umap_memory_start_snapshot);
#endif
	return OK;
}

static
RESULT
test_umap_alloc (void)
{
	dn_umap_t *map = dn_umap_alloc ();
	if (!map)
		return FAILED ("failed to alloc map");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_init (void)
{
	dn_umap_t map;
	if (!dn_umap_init (&map))
		return FAILED ("failed to init map");

	dn_umap_dispose (&map);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
umap_key_dispose_func (void *data)
{
	(*(uint32_t *)data)++;
}

static
void
DN_CALLBACK_CALLTYPE
umap_value_dispose_func (void *data)
{
	(*(uint32_t *)data)++;
}

static
RESULT
test_umap_free (void)
{
	uint32_t dispose_count = 0;
	dn_umap_t *map = dn_umap_custom_alloc (DN_DEFAULT_ALLOCATOR, NULL, NULL, NULL, umap_value_dispose_func);
	if (!map)
		return FAILED ("failed to custom alloc map");

	dn_umap_insert (map, INT32_TO_POINTER (1), &dispose_count);
	dn_umap_insert (map, INT32_TO_POINTER (2), &dispose_count);

	dn_umap_free (map);

	if (dispose_count != 2)
		return FAILED ("invalid dispose count on free");

	return OK;
}

static
RESULT
test_umap_dispose (void)
{
	uint32_t dispose_count = 0;
	dn_umap_t map;
	if (!dn_umap_custom_init (&map, DN_DEFAULT_ALLOCATOR, NULL, NULL, NULL, umap_value_dispose_func))
		return FAILED ("failed to custom init map");

	dn_umap_insert (&map, INT32_TO_POINTER (1), &dispose_count);
	dn_umap_insert (&map, INT32_TO_POINTER (2), &dispose_count);

	dn_umap_dispose (&map);

	if (dispose_count != 2)
		return FAILED ("invalid dispose count on free");

	return OK;
}

static
RESULT
test_umap_empty (void)
{
	const char * items[] = { "first", "second" };

	dn_umap_t *map = dn_umap_alloc ();
	if (!map)
		return FAILED ("failed to alloc map");

	if (!dn_umap_empty (map))
		return FAILED ("failed empty #1");

	dn_umap_insert (map, items [0], items [0]);

	if (dn_umap_empty (map))
		return FAILED ("failed empty #2");

	dn_umap_insert (map, items [1], items [1]);

	if (dn_umap_empty (map))
		return FAILED ("failed empty #3");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_size (void)
{
	dn_umap_t *map = dn_umap_alloc ();

	if (dn_umap_size (map) != 0)
		return FAILED ("map size didn't match");

	for (int32_t i = 0; i < 10; ++i)
		dn_umap_insert (map, INT32_TO_POINTER (i), NULL);

	if (dn_umap_size (map) != 10)
		return FAILED ("size failed #1");

	dn_umap_clear (map);

	if (dn_umap_size (map) != 0)
		return FAILED ("size failed #2");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_max_size (void)
{
	dn_umap_t *map = dn_umap_alloc ();
	if (!map)
		return FAILED ("failed to alloc map");

	if (dn_umap_max_size (map) != UINT32_MAX)
		return FAILED ("max_size failed");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_clear (void)
{
	uint32_t dispose_count = 0;
	const char * items[] = { "first", "second" };

	dn_umap_t *map = dn_umap_alloc ();
	if (!map)
		return FAILED ("failed to alloc map");

	if (!dn_umap_empty (map))
		return FAILED ("failed empty #1");

	dn_umap_insert (map, items [0], items [0]);

	if (dn_umap_empty (map))
		return FAILED ("failed empty #2");

	dn_umap_insert (map, items [1], items [1]);

	dn_umap_clear (map);

	if (!dn_umap_empty (map))
		return FAILED ("failed empty #3");

	dn_umap_free (map);

	map = dn_umap_custom_alloc (DN_DEFAULT_ALLOCATOR, NULL, NULL, NULL, umap_value_dispose_func);

	dn_umap_insert (map, INT32_TO_POINTER (1), &dispose_count);
	dn_umap_insert (map, INT32_TO_POINTER (2), &dispose_count);

	dn_umap_clear (map);

	if (dispose_count != 2)
		return FAILED ("invalid dispose count on clear");

	dispose_count = 0;
	dn_umap_free (map);

	if (dispose_count != 0)
		return FAILED ("invalid dispose count on clear/free");

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
umap_str_key_dispose_func (void *data)
{
	free (data);
}

static
RESULT
test_umap_insert (void)
{
	dn_umap_result_t result;
	const char *items[] = { "first", "second" };

	dn_umap_t *map = dn_umap_alloc ();
	result = dn_umap_insert (map, items [0], items [0]);
	if (!result.result || dn_umap_it_key (result.it) != items [0] || dn_umap_it_value (result.it) != items [0])
		return FAILED ("insert failed #1");

	result = dn_umap_insert (map, items [1], items [1]);
	if (!result.result || dn_umap_it_key (result.it) != items [1] || dn_umap_it_value (result.it) != items [1])
		return FAILED ("insert failed #2");

	result = dn_umap_insert (map, items [1], NULL);
	if (result.result || dn_umap_it_key (result.it) != items [1] || dn_umap_it_value (result.it) != items [1])
		return FAILED ("insert failed #3");

	dn_umap_free (map);

	map = dn_umap_custom_alloc (DN_DEFAULT_ALLOCATOR, dn_str_hash, dn_str_equal, umap_str_key_dispose_func, NULL);
	dn_umap_insert (map, strdup ("first"), items [0]);

	char *exists = strdup ("first");
	result = dn_umap_insert (map, exists, items [0]);
	if (result.result)
		return FAILED ("insert failed #4");
	free (exists);

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_insert_or_assign (void)
{
	dn_umap_result_t result;
	const char *items[] = { "first", "second" };

	dn_umap_t *map = dn_umap_alloc ();
	result = dn_umap_insert_or_assign (map, items [0], items [0]);
	if (!result.result || dn_umap_it_key (result.it) != items [0] || dn_umap_it_value (result.it) != items [0])
		return FAILED ("insert_or_assign failed #1");

	result = dn_umap_insert_or_assign (map, items [1], items [1]);
	if (!result.result || dn_umap_it_key (result.it) != items [1] || dn_umap_it_value (result.it) != items [1])
		return FAILED ("insert_or_assign failed #2");

	result = dn_umap_insert_or_assign (map, items [1], NULL);
	if (!result.result || dn_umap_it_key (result.it) != items [1] || dn_umap_it_value (result.it) != NULL)
		return FAILED ("insert_or_assign failed #3");

	dn_umap_free (map);

	map = dn_umap_custom_alloc (DN_DEFAULT_ALLOCATOR, dn_str_hash, dn_str_equal, umap_str_key_dispose_func, NULL);
	dn_umap_insert_or_assign (map, strdup ("first"), items [0]);

	result = dn_umap_insert_or_assign (map, strdup ("first"), items [1]);
	if (!result.result || strcmp (dn_umap_it_key (result.it), items [0]) || dn_umap_it_value (result.it) != items [1])
		return FAILED ("insert_or_assign failed #4");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_erase (void)
{
	const char *items[] = { "first", "second", "third", "fourth"};

	dn_umap_t *map = dn_umap_alloc ();

	dn_umap_insert (map, items [0], items [0]);
	dn_umap_insert (map, items [1], items [1]);

	dn_umap_it_t it = dn_umap_begin (map);
	char *key = dn_umap_it_key (it);
	char *value = dn_umap_it_value (it);

	dn_umap_it_t result = dn_umap_erase (it);
	if (dn_umap_size (map) != 1 || dn_umap_it_key (result) == key || dn_umap_it_value (result) == value)
		return FAILED ("erase failed #1");

	if (dn_umap_erase_key (map, NULL) != 0)
		return FAILED ("erase failed #2");

	dn_umap_insert (map, items [2], items [2]);
	dn_umap_insert (map, items [3], items [3]);

	if (dn_umap_erase_key (map, items [2]) == 0)
		return FAILED ("erase failed #3");

	result = dn_umap_erase (dn_umap_begin (map));
	result = dn_umap_erase (dn_umap_begin (map));

	if (!dn_umap_it_end (result))
		return FAILED ("erase failed #4");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_extract (void)
{
	const char *items[] = { "first", "second", "third", "fourth"};

	dn_umap_t *map = dn_umap_alloc ();

	dn_umap_insert (map, items [0], items [1]);
	dn_umap_insert (map, items [2], items [3]);

	char *key;
	char *value;

	if (!dn_umap_extract_key (map, items [0], &key, &value) || key != items [0] || value != items [1])
		return FAILED ("extract failed #1");

	if (dn_umap_size (map) != 1)
		return FAILED ("extract failed #2");

	dn_umap_free (map);

	uint32_t key_dispose_count = 0;
	uint32_t value_dispose_count = 0;
	map = dn_umap_custom_alloc (DN_DEFAULT_ALLOCATOR, NULL, NULL, umap_key_dispose_func, umap_value_dispose_func);

	dn_umap_insert (map, &key_dispose_count, &value_dispose_count);
	if (!dn_umap_extract_key (map, &key_dispose_count, NULL, NULL))
		return FAILED ("extract failed #3");

	if (key_dispose_count != 0 || value_dispose_count != 0 || dn_umap_size (map) != 0)
		return FAILED ("extract failed #4");

	dn_umap_free (map);

	return OK;
}

static
bool
DN_CALLBACK_CALLTYPE
umap_find_func (
	const void *a,
	const void *b)
{
	if (!a || !b)
		return false;

	return !strcmp ((const char *)a, (const char *)b);
}

static
RESULT
test_umap_find (void)
{
	const char *items[] = { "first", "second", "third", "fourth"};

	dn_umap_t *map = dn_umap_alloc ();

	dn_umap_insert (map, items [2], items [2]);
	dn_umap_insert (map, items [1], items [1]);
	dn_umap_insert (map, items [0], items [0]);

	char *data = items[3];
	dn_umap_insert (map, data, data);

	dn_umap_it_t found1 = dn_umap_find (map, data);
	dn_umap_it_t found2 = dn_umap_custom_find (map, data, umap_find_func);

	if (dn_umap_it_key_t (found1, char *) != data || dn_umap_it_key_t (found2, char *) != data)
		return FAILED ("find failed #1");

	found1 = dn_umap_find (map, NULL);
	found2 = dn_umap_custom_find (map, NULL, umap_find_func);
	if (!dn_umap_it_end (found1) || !dn_umap_it_end (found2))
		return FAILED ("find failed #2");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_contains (void)
{
	const char *items[] = { "first", "second", "third", "fourth"};

	dn_umap_t *map = dn_umap_alloc ();

	dn_umap_insert (map, items [2], items [2]);
	dn_umap_insert (map, items [1], items [1]);
	dn_umap_insert (map, items [0], items [0]);

	if (!dn_umap_contains (map, items [0]))
		return FAILED ("contains failed #1");

	if (dn_umap_contains (map, "unkown"))
		return FAILED ("contains failed #2");

	dn_umap_erase_key (map, items [1]);

	if (dn_umap_contains (map, items [1]))
		return FAILED ("contains failed #3");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_rehash (void)
{
	dn_umap_t *map = dn_umap_alloc ();

	for (uint32_t i = 0; i < 1000; i++)
		dn_umap_insert (map, INT32_TO_POINTER (i), INT32_TO_POINTER (i));

	dn_umap_rehash (map, dn_umap_size (map) * 2);

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_reserve (void)
{
	dn_umap_t *map = dn_umap_alloc ();

	dn_umap_reserve (map, 1000);

	for (uint32_t i = 0; i < 1000; i++)
		dn_umap_insert (map, INT32_TO_POINTER (i), INT32_TO_POINTER (i));

	dn_umap_free (map);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
umap_foreach_func (
	void *key,
	void *value,
	void *user_data)
{
	(*(uint32_t *)user_data)++;
}

static
RESULT
test_umap_for_each (void)
{
	uint32_t count = 0;
	dn_umap_t *map = dn_umap_alloc ();

	for (uint32_t i = 0; i < 100; ++i)
		dn_umap_insert (map, INT32_TO_POINTER (i), INT32_TO_POINTER (i));

	dn_umap_for_each (map, umap_foreach_func, &count);
	if (count != 100)
		return FAILED ("for_each failed");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_iterator (void)
{
	uint32_t count = 0;
	dn_umap_t *map = dn_umap_alloc ();

	for (uint32_t i = 0; i < 100; ++i)
		dn_umap_insert (map, INT32_TO_POINTER (i), INT32_TO_POINTER (i));

	DN_UMAP_FOREACH_BEGIN (map, uint32_t, key, uint32_t, value) {
		if (key == value)
			count++;
	} DN_UMAP_FOREACH_END;

	if (count != 100)
		return FAILED ("foreach iterator failed #2");

	count = 0;
	DN_UMAP_FOREACH_KEY_BEGIN (map, uint32_t, key) {
		count += key;
	} DN_UMAP_FOREACH_END;

	if (count != 4950)
		return FAILED ("foreach iterator failed #4");

	dn_umap_free (map);

	return OK;
}

static
RESULT
test_umap_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_umap_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_umap_memory_diff_snapshot, &dn_umap_memory_start_snapshot, &dn_umap_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_umap_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_umap_tests [] = {
	{"test_umap_setup", test_umap_setup},
	{"test_umap_alloc", test_umap_alloc},
	{"test_umap_init", test_umap_init},
	{"test_umap_free", test_umap_free},
	{"test_umap_dispose", test_umap_dispose},
	{"test_umap_empty", test_umap_empty},
	{"test_umap_size", test_umap_size},
	{"test_umap_max_size", test_umap_max_size},
	{"test_umap_clear", test_umap_clear},
	{"test_umap_insert", test_umap_insert},
	{"test_umap_insert_or_assign", test_umap_insert_or_assign},
	{"test_umap_erase", test_umap_erase},
	{"test_umap_extract", test_umap_extract},
	{"test_umap_find", test_umap_find},
	{"test_umap_contains", test_umap_contains},
	{"test_umap_rehash", test_umap_rehash},
	{"test_umap_reserve", test_umap_reserve},
	{"test_umap_for_each", test_umap_for_each},
	{"test_umap_iterator", test_umap_iterator},
	{"test_umap_teardown", test_umap_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_umap_tests_init, dn_umap_tests)
