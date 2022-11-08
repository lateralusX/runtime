#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-slist-ex.h>


#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_slist_memory_start_snapshot;
static _CrtMemState dn_slist_memory_end_snapshot;
static _CrtMemState dn_slist_memory_diff_snapshot;
#endif

#define N_ELEMS 100
#define POINTER_TO_INT32(v) ((int32_t)(ptrdiff_t)(v))
#define INT32_TO_POINTER(v) ((void *)(ptrdiff_t)(v))

static
RESULT
test_slist_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_slist_memory_start_snapshot);
#endif
	return OK;
}

static
RESULT
test_slist_length (void)
{
	dn_slist_t *list = dn_slist_prepend (NULL, (char*)"foo");

	if (dn_slist_length (list) != 1)
		return FAILED ("length failed. #1");

	list = dn_slist_prepend (list, (char*)"bar");
	if (dn_slist_length (list) != 2)
		return FAILED ("length failed. #2");

	list = dn_slist_append (list, (char*)"bar");
	if (dn_slist_length (list) != 3)
		return FAILED ("length failed. #3");

	dn_slist_free (list);
	return NULL;
}

static
RESULT
test_slist_nth (void)
{
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";
	dn_slist_t *nth, *list;
	list = dn_slist_prepend (NULL, baz);
	list = dn_slist_prepend (list, bar);
	list = dn_slist_prepend (list, foo);

	nth = dn_slist_nth (list, 0);
	if (nth->data != foo)
		return FAILED ("nth failed. #0");

	nth = dn_slist_nth (list, 1);
	if (nth->data != bar)
		return FAILED ("nth failed. #1");

	nth = dn_slist_nth (list, 2);
	if (nth->data != baz)
		return FAILED ("nth failed. #2");

	nth = dn_slist_nth (list, 3);
	if (nth)
		return FAILED ("nth failed. #3: %s", nth->data);

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_nth_data (void)
{
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";
	dn_slist_t *list;
	void *nth_data;

	list = dn_slist_prepend (NULL, baz);
	list = dn_slist_prepend (list, bar);
	list = dn_slist_prepend (list, foo);

	nth_data = dn_slist_nth_data (list, 0);
	if (nth_data != foo)
		return FAILED ("nth_data failed. #0");

	nth_data = dn_slist_nth_data (list, 1);
	if (nth_data != bar)
		return FAILED ("nth_data failed. #1");

	nth_data = dn_slist_nth_data (list, 2);
	if (nth_data != baz)
		return FAILED ("nth_data failed. #2");

	nth_data = dn_slist_nth_data (list, 3);
	if (nth_data)
		return FAILED ("nth_data failed. #3: %s", nth_data);

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_index (void)
{
	int32_t i;
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";
	dn_slist_t *list;
	list = dn_slist_prepend (NULL, baz);
	list = dn_slist_prepend (list, bar);
	list = dn_slist_prepend (list, foo);

	i = dn_slist_index (list, foo);
	if (i != 0)
		return FAILED ("index failed. #0: %d", i);

	i = dn_slist_index (list, bar);
	if (i != 1)
		return FAILED ("index failed. #1: %d", i);

	i = dn_slist_index (list, baz);
	if (i != 2)
		return FAILED ("index failed. #2: %d", i);

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_append (void)
{
	dn_slist_t *foo;
	dn_slist_t *list = dn_slist_append (NULL, (char*)"first");
	if (dn_slist_length (list) != 1)
		return FAILED ("append(null,...) failed");

	foo = dn_slist_append (list, (char*)"second");
	if (foo != list)
		return FAILED ("changed list head on non-empty");

	if (dn_slist_length (list) != 2)
		return FAILED ("append failed");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_concat (void)
{
	dn_slist_t *foo = dn_slist_prepend (NULL, (char*)"foo");
	dn_slist_t *bar = dn_slist_prepend (NULL, (char*)"bar");

	dn_slist_t *list = dn_slist_concat (foo, bar);

	if (dn_slist_length (list) != 2)
		return FAILED ("concat failed.");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_find (void)
{
	dn_slist_t *list = dn_slist_prepend (NULL, (char*)"three");
	dn_slist_t *found;
	char *data;

	list = dn_slist_prepend (list, (char*)"two");
	list = dn_slist_prepend (list, (char*)"one");

	data = (char*)"four";
	list = dn_slist_append (list, data);

	found = dn_slist_find (list, data);

	if (found->data != data)
		return FAILED ("find failed");

	dn_slist_free (list);
	return OK;
}

static
int32_t
DN_CALLBACK_CALLTYPE
find_custom (const void *a, const void *b)
{
	return(strcmp ((const char *)a, (const char *)b));
}

static
RESULT
test_slist_find_custom (void)
{
	dn_slist_t *list = NULL, *found;
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";

	list = dn_slist_prepend (list, baz);
	list = dn_slist_prepend (list, bar);
	list = dn_slist_prepend (list, foo);

	found = dn_slist_find_custom (list, baz, find_custom);

	if (found == NULL)
		return FAILED ("find failed");

	dn_slist_free (list);

	return OK;
}

static
RESULT
test_slist_remove (void)
{
	dn_slist_t *list = dn_slist_prepend (NULL,(char*) "three");
	char *one = (char*)"one";
	list = dn_slist_prepend (list, (char*)"two");
	list = dn_slist_prepend (list, one);

	list = dn_slist_remove (list, one);

	if (dn_slist_length (list) != 2)
		return FAILED ("remove failed");

	if (strcmp ("two", list->data) != 0)
		return FAILED ("Remove failed");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_remove_link (void)
{
	dn_slist_t *foo = dn_slist_prepend (NULL, (char*)"a");
	dn_slist_t *bar = dn_slist_prepend (NULL, (char*)"b");
	dn_slist_t *baz = dn_slist_prepend (NULL, (char*)"c");
	dn_slist_t *list = foo;

	foo = dn_slist_concat (foo, bar);
	foo = dn_slist_concat (foo, baz);

	list = dn_slist_remove_link (list, bar);

	if (dn_slist_length (list) != 2)
		return FAILED ("remove_link failed #1");

	if (bar->next != NULL)
		return FAILED ("remove_link failed #2");

	dn_slist_free (list);
	dn_slist_free (bar);

	return OK;
}

static
int32_t
DN_CALLBACK_CALLTYPE
compare (
	const void *a,
	const void *b)
{
	char *foo = (char *) a;
	char *bar = (char *) b;

	if (strlen (foo) < strlen (bar))
		return -1;

	return 1;
}

static
RESULT
test_slist_insert_sorted (void)
{
	dn_slist_t *list = dn_slist_prepend (NULL,(char*) "a");
	list = dn_slist_append (list, (char*)"aaa");

	/* insert at the middle */
	list = dn_slist_insert_sorted (list, (char*)"aa", compare);
	if (strcmp ("aa", list->next->data))
		return FAILED("insert_sorted failed #1");

	/* insert at the beginning */
	list = dn_slist_insert_sorted (list, (char*)"", compare);
	if (strcmp ("", list->data))
		return FAILED ("insert_sorted failed #2");

	/* insert at the end */
	list = dn_slist_insert_sorted (list, (char*)"aaaa", compare);
	if (strcmp ("aaaa", dn_slist_last (list)->data))
		return FAILED ("insert_sorted failed #3");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_insert_before (void)
{
	dn_slist_t *foo, *bar, *baz;

	foo = dn_slist_prepend (NULL, (char*)"foo");
	foo = dn_slist_insert_before (foo, NULL, (char*)"bar");
	bar = dn_slist_last (foo);

	if (strcmp (bar->data, "bar"))
		return FAILED ("1");

	baz = dn_slist_insert_before (foo, bar, (char*)"baz");
	if (foo != baz)
		return FAILED ("2");

	if (strcmp (foo->next->data, "baz"))
		return FAILED ("3: %s", foo->next->data);

	dn_slist_free (foo);
	return OK;
}

static
int32_t
DN_CALLBACK_CALLTYPE
intcompare (
	const void *p1,
	const void *p2)
{
	return POINTER_TO_INT32 (p1) - POINTER_TO_INT32 (p2);
}

static
bool
verify_sort (
	dn_slist_t *list,
	int32_t len)
{
	int32_t prev = POINTER_TO_INT32 (list->data);
	len--;
	for (list = list->next; list; list = list->next) {
		int32_t curr = POINTER_TO_INT32 (list->data);
		if (prev > curr)
			return false;
		prev = curr;

		if (len == 0)
			return false;
		len--;
	}
	return len == 0;
}

static
RESULT
test_slist_sort (void)
{
	int32_t i, j, mul;
	dn_slist_t *list = NULL;

	for (i = 0; i < N_ELEMS; ++i)
		list = dn_slist_prepend (list, INT32_TO_POINTER (i));
	list = dn_slist_sort (list, intcompare);
	if (!verify_sort (list, N_ELEMS))
		return FAILED ("decreasing list");

	dn_slist_free (list);

	list = NULL;
	for (i = 0; i < N_ELEMS; ++i)
		list = dn_slist_prepend (list, INT32_TO_POINTER (-i));
	list = dn_slist_sort (list, intcompare);
	if (!verify_sort (list, N_ELEMS))
		return FAILED ("increasing list");

	dn_slist_free (list);

	list = dn_slist_prepend (NULL, INT32_TO_POINTER (0));
	for (i = 1; i < N_ELEMS; ++i) {
		list = dn_slist_prepend (list, INT32_TO_POINTER (-i));
		list = dn_slist_prepend (list, INT32_TO_POINTER (i));
	}
	list = dn_slist_sort (list, intcompare);
	if (!verify_sort (list, 2*N_ELEMS-1))
		return FAILED ("alternating list");

	dn_slist_free (list);

	list = NULL;
	mul = 1;
	for (i = 1; i < N_ELEMS; ++i) {
		mul = -mul;
		for (j = 0; j < i; ++j)
			list = dn_slist_prepend (list, INT32_TO_POINTER (mul * j));
	}
	list = dn_slist_sort (list, intcompare);
	if (!verify_sort (list, (N_ELEMS*N_ELEMS - N_ELEMS)/2))
		return FAILED ("wavering list");

	dn_slist_free (list);

	return OK;
}

static
RESULT
test_slist_copy (void)
{
	dn_slist_t *list;
	list = dn_slist_prepend (NULL, (char*)"baz");
	list = dn_slist_prepend (list, (char*)"bar");
	list = dn_slist_prepend (list, (char*)"foo");

	dn_slist_t *copy = dn_slist_copy (list);
	if (dn_slist_length (list) != dn_slist_length (copy))
		return FAILED ("size diff, copy failed");

	for (dn_slist_t *list_it = list, *copy_it = copy; list_it && copy_it; list_it = list_it->next, copy_it = copy_it->next) {
		if (strcmp((char *)list_it->data, (char *)copy_it->data))
			return FAILED ("data diff, copy failed");
	}

	dn_slist_free (copy);
	dn_slist_free (list);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
foreach_func (
	void *data,
	void *user_data)
{
	(*(uint32_t *)user_data)++;
}

static
RESULT
test_slist_foreach (void)
{
	uint32_t count = 0;
	dn_slist_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_slist_prepend (list, INT32_TO_POINTER (i));

	dn_slist_foreach (list, foreach_func, &count);
	if (count != dn_slist_length (list))
		return FAILED ("foreach failed");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_remove_all (void)
{
	const uint32_t count = N_ELEMS * 2;
	dn_slist_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_slist_prepend (list, INT32_TO_POINTER (i));

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_slist_prepend (list, INT32_TO_POINTER (i));

	list = dn_slist_remove_all (list, INT32_TO_POINTER (0));

	if (dn_slist_length (list) != count - 2)
		return FAILED ("remove_all failed #1");

	list = dn_slist_remove_all (list, INT32_TO_POINTER (N_ELEMS - 1));

	if (dn_slist_length (list) != count - 4)
		return FAILED ("remove_all failed #2");

	// Not found.
	list = dn_slist_remove_all (list, INT32_TO_POINTER (count * 3));
	if (dn_slist_length (list) != count - 4)
		return FAILED ("remove_all failed #3");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_reverse (void)
{
	uint32_t count = N_ELEMS;
	dn_slist_t *list = NULL;
	dn_slist_t *reverse = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_slist_append (list, INT32_TO_POINTER (i));

	reverse = dn_slist_reverse (list);

	DN_SLIST_EX_FOREACH_BEGIN (reverse, void *, value) {
		if (POINTER_TO_INT32 (value) != count - 1)
			return FAILED ("reverse failed #1");
		count--;
	} DN_SLIST_EX_FOREACH_END;

	if (count != 0)
		return FAILED ("reverse failed #2");

	dn_slist_free (reverse);

	return OK;
}

static
RESULT
test_slist_delete_link (void)
{
	dn_slist_t *foo = dn_slist_prepend (NULL, (char*)"a");
	dn_slist_t *bar = dn_slist_prepend (NULL, (char*)"b");
	dn_slist_t *baz = dn_slist_prepend (NULL, (char*)"c");
	dn_slist_t *list = foo;

	foo = dn_slist_concat (foo, bar);
	foo = dn_slist_concat (foo, baz);

	list = dn_slist_delete_link (list, bar);

	if (dn_slist_length (list) != 2)
		return FAILED ("delete_link failed #1");

	dn_slist_free (list);

	return OK;
}

static
RESULT
test_slist_next (void)
{
	uint32_t count = 0;
	dn_slist_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i) {
		list = dn_slist_append (list, INT32_TO_POINTER (i));
		count++;
	}

	for (dn_slist_t * it = list; it; it = dn_slist_next (it))
		count--;

	if (count != 0)
		return FAILED ("next failed");

	dn_slist_free (list);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
free_callback(void *data)
{
	free (data);
}

static
RESULT
test_slist_ex_free (void)
{
	dn_slist_t *list = dn_slist_append (NULL, strdup ("a"));
	list = dn_slist_append (list, strdup ("b"));
	list = dn_slist_append (list, strdup ("c"));

	dn_slist_ex_for_each_free (&list, free_callback);
	if (list != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
foreach_clear_free_callback (void *data)
{
	*((uint32_t *)data) = 0;
}

static
RESULT
test_slist_ex_foreach_free (void)
{
	uint32_t value1 = 1;
	uint32_t value2 = 2;
	uint32_t value3 = 3;

	dn_slist_t *list = dn_slist_append (NULL, &value1);
	list = dn_slist_append (list, &value2);
	list = dn_slist_append (list, &value3);

	dn_slist_ex_for_each_free (&list, foreach_clear_free_callback);
	if (list != NULL)
		return FAILED ("ex_foreach_free failed #1");

	if (value1 != 0 || value2 != 0 || value3 != 0)
		return FAILED ("ex_foreach_free failed #2");

	return OK;
}

static
RESULT
test_slist_ex_clear(void)
{
	uint32_t value1 = 1;
	uint32_t value2 = 2;
	uint32_t value3 = 3;

	dn_slist_t *list = dn_slist_append (NULL, &value1);
	list = dn_slist_append (list, &value2);
	list = dn_slist_append (list, &value3);

	dn_slist_ex_clear (&list);

	if (!dn_slist_ex_empty (list))
		return FAILED ("ex_clear failed");

	return OK;
}

static
RESULT
test_slist_ex_foreach_clear(void)
{
	uint32_t value1 = 1;
	uint32_t value2 = 2;
	uint32_t value3 = 3;

	dn_slist_t *list = dn_slist_append (NULL, &value1);
	list = dn_slist_append (list, &value2);
	list = dn_slist_append (list, &value3);

	dn_slist_ex_for_each_clear (&list, foreach_clear_free_callback);
	if (list != NULL)
		return FAILED ("ex_foreach_clear failed #1");

	if (value1 != 0 || value2 != 0 || value3 != 0)
		return FAILED ("ex_foreach_clear failed #2");

	return OK;
}

static
RESULT
test_slist_ex_foreach_it (void)
{
	uint32_t count = 0;
	dn_slist_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_slist_append (list, INT32_TO_POINTER (i));

	DN_SLIST_EX_FOREACH_BEGIN (list, void *, value) {
		if (POINTER_TO_INT32 (value) != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_SLIST_EX_FOREACH_END;

	if (count != dn_slist_length (list))
		return FAILED ("foreach iterator failed #2");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_ex_push_back (void)
{
	dn_slist_t *list = NULL;
	dn_slist_ex_push_back (&list, strdup ("a"));

	dn_slist_t *list_2 = list;
	dn_slist_ex_push_back (&list_2, strdup ("b"));
	if (list != list_2)
		return FAILED ("ex_push_back failed #1");

	dn_slist_t *list_3 = list_2;
	dn_slist_ex_push_back (&list_3, strdup ("c"));
	if (list_2 != list_3)
		return FAILED ("ex_push_back failed #2");

	dn_slist_ex_for_each_free (&list_3, free_callback);
	if (list_3 != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_slist_ex_push_front (void)
{
	dn_slist_t *list = NULL;
	dn_slist_ex_push_front (&list, strdup ("a"));

	dn_slist_t *list_2 = list;
	dn_slist_ex_push_front (&list_2, strdup ("b"));
	if (list == list_2)
		return FAILED ("ex_push_back failed #1");

	dn_slist_t *list_3 = list_2;
	dn_slist_ex_push_front (&list_3, strdup ("c"));
	if (list_2 == list_3)
		return FAILED ("ex_push_back failed #2");

	char * value = dn_slist_ex_data (list_3, char *);
	if (!value || strcmp (value, "c"))
		return FAILED ("ex_push_front failed #3");

	dn_slist_ex_for_each_free (&list_3, free_callback);
	if (list_3 != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_slist_ex_erase (void)
{
	const char *a = "a";

	dn_slist_t *list = dn_slist_append (NULL, (void *)a);
	list = dn_slist_append (list, strdup ("b"));
	list = dn_slist_append (list, strdup ("c"));

	dn_slist_t *list_2 = list;
	dn_slist_ex_erase (&list_2, a);
	if (list == list_2)
		return FAILED ("ex_remove failed");

	dn_slist_ex_for_each_free (&list_2, free_callback);
	if (list_2 != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_slist_ex_find (void)
{
	dn_slist_t *list = dn_slist_prepend (NULL, (char*)"three");
	bool result = false;
	dn_slist_t *found;
	char *data;

	list = dn_slist_prepend (list, (char*)"two");
	list = dn_slist_prepend (list, (char*)"one");

	data = (char*)"four";
	list = dn_slist_append (list, data);

	result = dn_slist_ex_find (list, data, &found);

	if (!result || found->data != data)
		return FAILED ("find failed");

	dn_slist_free (list);
	return OK;
}

static
RESULT
test_slist_ex_find_custom (void)
{
	dn_slist_t *list = NULL, *found;
	bool result = false;
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";

	list = dn_slist_prepend (list, baz);
	list = dn_slist_prepend (list, bar);
	list = dn_slist_prepend (list, foo);

	result = dn_slist_ex_find_custom (list, baz, find_custom, &found);

	if (!result ||found == NULL)
		return FAILED ("Find failed");

	dn_slist_free (list);

	return OK;
}

static
RESULT
test_slist_ex_empty (void)
{
	dn_slist_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_slist_append (list, INT32_TO_POINTER (i));

	if (dn_slist_ex_empty (list))
		return FAILED ("is_empty failed #1");

	dn_slist_ex_free (&list);

	if (!dn_slist_ex_empty (list))
		return FAILED ("is_empty failed #2");

	return OK;
}

static
RESULT
test_slist_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_slist_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_slist_memory_diff_snapshot, &dn_slist_memory_start_snapshot, &dn_slist_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_slist_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_slist_tests [] = {
	{"test_slist_setup", test_slist_setup},
	{"test_slist_length", test_slist_length},
	{"test_slist_nth", test_slist_nth},
	{"test_slist_nth_data", test_slist_nth_data},
	{"test_slist_index", test_slist_index},
	{"test_slist_append", test_slist_append},
	{"test_slist_concat", test_slist_concat},
	{"test_slist_find", test_slist_find},
	{"test_slist_find_custom", test_slist_find_custom},
	{"test_slist_remove", test_slist_remove},
	{"test_slist_remove_link", test_slist_remove_link},
	{"test_slist_insert_sorted", test_slist_insert_sorted},
	{"test_slist_insert_before", test_slist_insert_before},
	{"test_slist_sort", test_slist_sort},
	{"test_slist_copy", test_slist_copy},
	{"test_slist_foreach", test_slist_foreach},
	{"test_slist_remove_all", test_slist_remove_all},
	{"test_slist_reverse", test_slist_reverse},
	{"test_slist_delete_link", test_slist_delete_link},
	{"test_slist_next", test_slist_next},
	{"test_slist_ex_free",test_slist_ex_free},
	{"test_slist_ex_foreach_free",test_slist_ex_foreach_free},
	{"test_slist_ex_foreach_clear",test_slist_ex_foreach_clear},
	{"test_slist_ex_foreach_it",test_slist_ex_foreach_it},
	{"test_slist_ex_push_back",test_slist_ex_push_back},
	{"test_slist_ex_push_front",test_slist_ex_push_front},
	{"test_slist_ex_erase",test_slist_ex_erase},
	{"test_slist_ex_find",test_slist_ex_find},
	{"test_slist_ex_find_custom",test_slist_ex_find_custom},
	{"test_slist_ex_empty",test_slist_ex_empty},
	{"test_slist_teardown", test_slist_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_slist_tests_init, dn_slist_tests)
