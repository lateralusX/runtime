#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <eglib/test/test.h>
#include <containers/dn-list-ex.h>


#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState dn_list_memory_start_snapshot;
static _CrtMemState dn_list_memory_end_snapshot;
static _CrtMemState dn_list_memory_diff_snapshot;
#endif

#define N_ELEMS 101
#define POINTER_TO_INT32(v) ((int32_t)(ptrdiff_t)(v))
#define INT32_TO_POINTER(v) ((void *)(ptrdiff_t)(v))

static
RESULT
test_list_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_list_memory_start_snapshot);
#endif
	return OK;
}

static
RESULT
test_list_length (void)
{
	dn_list_t *list = dn_list_prepend (NULL, (char*)"foo");

	if (dn_list_length (list) != 1)
		return FAILED ("length failed. #1");

	list = dn_list_prepend (list, (char*)"bar");
	if (dn_list_length (list) != 2)
		return FAILED ("length failed. #2");

	list = dn_list_append (list, (char*)"bar");
	if (dn_list_length (list) != 3)
		return FAILED ("length failed. #3");

	dn_list_free (list);
	return NULL;
}

static
RESULT
test_list_nth (void)
{
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";
	dn_list_t *nth, *list;
	list = dn_list_prepend (NULL, baz);
	list = dn_list_prepend (list, bar);
	list = dn_list_prepend (list, foo);

	nth = dn_list_nth (list, 0);
	if (nth->data != foo)
		return FAILED ("nth failed. #0");

	nth = dn_list_nth (list, 1);
	if (nth->data != bar)
		return FAILED ("nth failed. #1");

	nth = dn_list_nth (list, 2);
	if (nth->data != baz)
		return FAILED ("nth failed. #2");

	nth = dn_list_nth (list, 3);
	if (nth)
		return FAILED ("nth failed. #3: %s", nth->data);

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_nth_data (void)
{
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";
	dn_list_t *list;
	void *nth_data;

	list = dn_list_prepend (NULL, baz);
	list = dn_list_prepend (list, bar);
	list = dn_list_prepend (list, foo);

	nth_data = dn_list_nth_data (list, 0);
	if (nth_data != foo)
		return FAILED ("nth_data failed. #0");

	nth_data = dn_list_nth_data (list, 1);
	if (nth_data != bar)
		return FAILED ("nth_data failed. #1");

	nth_data = dn_list_nth_data (list, 2);
	if (nth_data != baz)
		return FAILED ("nth_data failed. #2");

	nth_data = dn_list_nth_data (list, 3);
	if (nth_data)
		return FAILED ("nth_data failed. #3: %s", nth_data);

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_index (void)
{
	int i;
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";
	dn_list_t *list;
	list = dn_list_prepend (NULL, baz);
	list = dn_list_prepend (list, bar);
	list = dn_list_prepend (list, foo);

	i = dn_list_index (list, foo);
	if (i != 0)
		return FAILED ("index failed. #0: %d", i);

	i = dn_list_index (list, bar);
	if (i != 1)
		return FAILED ("index failed. #1: %d", i);

	i = dn_list_index (list, baz);
	if (i != 2)
		return FAILED ("index failed. #2: %d", i);

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_append (void)
{
	dn_list_t *list = dn_list_prepend (NULL, (char*)"first");
	if (dn_list_length (list) != 1)
		return FAILED ("Prepend failed");

	list = dn_list_append (list, (char*)"second");

	if (dn_list_length (list) != 2)
		return FAILED ("Append failed");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_last (void)
{
	dn_list_t *foo = dn_list_prepend (NULL, (char*)"foo");
	dn_list_t *bar = dn_list_prepend (NULL, (char*)"bar");
	dn_list_t *last;

	foo = dn_list_concat (foo, bar);
	last = dn_list_last (foo);

	if (last != bar)
		return FAILED ("last failed. #1");

	foo = dn_list_concat (foo, dn_list_prepend (NULL, (char*)"baz"));
	foo = dn_list_concat (foo, dn_list_prepend (NULL, (char*)"quux"));

	last = dn_list_last (foo);
	if (strcmp ("quux", last->data))
		return FAILED ("last failed. #2");

	dn_list_free (foo);

	return OK;
}

static
RESULT
test_list_concat (void)
{
	dn_list_t *foo = dn_list_prepend (NULL, (char*)"foo");
	dn_list_t *bar = dn_list_prepend (NULL, (char*)"bar");
	dn_list_t *list = dn_list_concat (foo, bar);

	if (dn_list_length (list) != 2)
		return FAILED ("Concat failed. #1");

	if (strcmp (list->data, "foo"))
		return FAILED ("Concat failed. #2");

	if (strcmp (list->next->data, "bar"))
		return FAILED ("Concat failed. #3");

	if (dn_list_first (list) != foo)
		return FAILED ("Concat failed. #4");

	if (dn_list_last (list) != bar)
		return FAILED ("Concat failed. #5");

	dn_list_free (list);

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
test_list_insert_sorted (void)
{
	dn_list_t *list = dn_list_prepend (NULL, (char*)"a");
	list = dn_list_append (list, (char*)"aaa");

	/* insert at the middle */
	list = dn_list_insert_sorted (list, (char*)"aa", compare);
	if (strcmp ("aa", list->next->data))
		return FAILED ("insert_sorted failed. #1");

	/* insert at the beginning */
	list = dn_list_insert_sorted (list, (char*)"", compare);
	if (strcmp ("", list->data))
		return FAILED ("insert_sorted failed. #2");

	/* insert at the end */
	list = dn_list_insert_sorted (list, (char*)"aaaa", compare);
	if (strcmp ("aaaa", dn_list_last (list)->data))
		return FAILED ("insert_sorted failed. #3");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_copy (void)
{
	int32_t i, length;
	dn_list_t *list, *copy;
	list = dn_list_prepend (NULL, (char*)"a");
	list = dn_list_append  (list, (char*)"aa");
	list = dn_list_append  (list, (char*)"aaa");
	list = dn_list_append  (list, (char*)"aaaa");

	length = dn_list_length (list);
	copy = dn_list_copy (list);

	for (i = 0; i < length; i++)
		if (strcmp (dn_list_nth (list, i)->data, dn_list_nth (copy, i)->data))
			return FAILED ("copy failed.");

	dn_list_free (list);
	dn_list_free (copy);
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
test_list_foreach (void)
{
	uint32_t count = 0;
	dn_list_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_list_prepend (list, INT32_TO_POINTER (i));

	dn_list_foreach (list, foreach_func, &count);
	if (count != dn_list_length (list))
		return FAILED ("foreach failed");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_remove_all (void)
{
	const uint32_t count = N_ELEMS * 2;
	dn_list_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_list_prepend (list, INT32_TO_POINTER (i));

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_list_prepend (list, INT32_TO_POINTER (i));

	list = dn_list_remove_all (list, INT32_TO_POINTER (0));

	if (dn_list_length (list) != count - 2)
		return FAILED ("remove_all failed #1");

	list = dn_list_remove_all (list, INT32_TO_POINTER (N_ELEMS - 1));

	if (dn_list_length (list) != count - 4)
		return FAILED ("remove_all failed #2");

	// Not found.
	list = dn_list_remove_all (list, INT32_TO_POINTER (count * 3));
	if (dn_list_length (list) != count - 4)
		return FAILED ("remove_all failed #3");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_reverse (void)
{
	uint32_t i, length;
	dn_list_t *list, *reverse;
	list = dn_list_prepend (NULL, (char*)"a");
	list = dn_list_append  (list, (char*)"aa");
	list = dn_list_append  (list, (char*)"aaa");
	list = dn_list_append  (list, (char*)"aaaa");

	length  = dn_list_length (list);
	reverse = dn_list_reverse (dn_list_copy (list));

	if (dn_list_length (reverse) != length)
		return FAILED ("reverse failed #1");

	for (i = 0; i < length; i++){
		uint32_t j = length - i - 1;
		if (strcmp (dn_list_nth (list, i)->data, dn_list_nth (reverse, j)->data))
			return FAILED ("reverse failed. #2");
	}

	dn_list_free (list);
	dn_list_free (reverse);
	return OK;
}

static
RESULT
test_list_delete_link (void)
{
	dn_list_t *foo = dn_list_prepend (NULL, (char*)"a");
	dn_list_t *bar = dn_list_prepend (NULL, (char*)"b");
	dn_list_t *baz = dn_list_prepend (NULL, (char*)"c");
	dn_list_t *list = foo;

	foo = dn_list_concat (foo, bar);
	foo = dn_list_concat (foo, baz);

	list = dn_list_delete_link (list, bar);

	if (dn_list_length (list) != 2)
		return FAILED ("delete_link failed #1");

	dn_list_free (list);

	return OK;
}

static
RESULT
test_list_next (void)
{
	uint32_t count = 0;
	dn_list_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i) {
		list = dn_list_append (list, INT32_TO_POINTER (i));
		count++;
	}

	for (dn_list_t * it = list; it; it = dn_list_next (it))
		count--;

	if (count != 0)
		return FAILED ("next failed");

	dn_list_free (list);

	return OK;
}

static
RESULT
test_list_remove (void)
{
	dn_list_t *list = dn_list_prepend (NULL, (char*)"three");
	char *one = (char*)"one";
	list = dn_list_prepend (list, (char*)"two");
	list = dn_list_prepend (list, one);

	list = dn_list_remove (list, one);

	if (dn_list_length (list) != 2)
		return FAILED ("Remove failed");

	if (strcmp ("two", list->data) != 0)
		return FAILED ("Remove failed");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_remove_link (void)
{
	dn_list_t *foo = dn_list_prepend (NULL, (char*)"a");
	dn_list_t *bar = dn_list_prepend (NULL, (char*)"b");
	dn_list_t *baz = dn_list_prepend (NULL, (char*)"c");
	dn_list_t *list = foo;

	foo = dn_list_concat (foo, bar);
	foo = dn_list_concat (foo, baz);

	list = dn_list_remove_link (list, bar);

	if (dn_list_length (list) != 2)
		return FAILED ("remove_link failed #1");

	if (bar->next != NULL)
		return FAILED ("remove_link failed #2");

	dn_list_free (list);
	dn_list_free (bar);
	return OK;
}

static
RESULT
test_list_insert_before (void)
{
	dn_list_t *foo, *bar, *baz;

	foo = dn_list_prepend (NULL, (char*)"foo");
	foo = dn_list_insert_before (foo, NULL, (char*)"bar");
	bar = dn_list_last (foo);

	if (strcmp (bar->data, (char*)"bar"))
		return FAILED ("1");

	baz = dn_list_insert_before (foo, bar, (char*)"baz");
	if (foo != baz)
		return FAILED ("2");

	if (strcmp (dn_list_nth_data (foo, 1), (char*)"baz"))
		return FAILED ("3: %s", dn_list_nth_data (foo, 1));

	dn_list_free (foo);
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
	dn_list_t *list,
	int32_t len)
{
	int32_t prev;

	if (list->prev)
		return FALSE;

	prev = POINTER_TO_INT32 (list->data);
	len--;
	for (list = list->next; list; list = list->next) {
		int32_t curr = POINTER_TO_INT32 (list->data);
		if (prev > curr)
			return FALSE;
		prev = curr;

		if (!list->prev || list->prev->next != list)
			return FALSE;

		if (len == 0)
			return FALSE;
		len--;
	}
	return len == 0;
}

static
RESULT
test_list_sort (void)
{
	int32_t i, j, mul;
	dn_list_t *list = NULL;

	for (i = 0; i < N_ELEMS; ++i)
		list = dn_list_prepend (list, INT32_TO_POINTER (i));
	list = dn_list_sort (list, intcompare);
	if (!verify_sort (list, N_ELEMS))
		return FAILED ("decreasing list");

	dn_list_free (list);

	list = NULL;
	for (i = 0; i < N_ELEMS; ++i)
		list = dn_list_prepend (list, INT32_TO_POINTER (-i));
	list = dn_list_sort (list, intcompare);
	if (!verify_sort (list, N_ELEMS))
		return FAILED ("increasing list");

	dn_list_free (list);

	list = dn_list_prepend (NULL, INT32_TO_POINTER (0));
	for (i = 1; i < N_ELEMS; ++i) {
		list = dn_list_prepend (list, INT32_TO_POINTER (i));
		list = dn_list_prepend (list, INT32_TO_POINTER (-i));
	}
	list = dn_list_sort (list, intcompare);
	if (!verify_sort (list, 2*N_ELEMS-1))
		return FAILED ("alternating list");

	dn_list_free (list);

	list = NULL;
	mul = 1;
	for (i = 1; i < N_ELEMS; ++i) {
		mul = -mul;
		for (j = 0; j < i; ++j)
			list = dn_list_prepend (list, INT32_TO_POINTER (mul * j));
	}
	list = dn_list_sort (list, intcompare);
	if (!verify_sort (list, (N_ELEMS*N_ELEMS - N_ELEMS)/2))
		return FAILED ("wavering list");

	dn_list_free (list);

	return OK;
}

static
RESULT
test_list_find (void)
{
	dn_list_t *list = dn_list_prepend (NULL, (char*)"three");
	dn_list_t *found;
	char *data;

	list = dn_list_prepend (list, (char*)"two");
	list = dn_list_prepend (list, (char*)"one");

	data = (char*)"four";
	list = dn_list_append (list, data);

	found = dn_list_find (list, data);

	if (found->data != data)
		return FAILED ("find failed");

	dn_list_free (list);
	return OK;
}

static
int32_t
DN_CALLBACK_CALLTYPE
find_custom (
	const void *a,
	const void *b)
{
	return (strcmp (a, b));
}

static
RESULT
test_list_find_custom (void)
{
	dn_list_t *list = NULL, *found;
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";

	list = dn_list_prepend (list, baz);
	list = dn_list_prepend (list, bar);
	list = dn_list_prepend (list, foo);

	found = dn_list_find_custom (list, baz, find_custom);

	if (found == NULL)
		return FAILED ("Find failed");

	dn_list_free (list);

	return OK;
}

static
void
DN_CALLBACK_CALLTYPE
free_callback (void *data)
{
	free (data);
}

static
RESULT
test_list_ex_free (void)
{
	dn_list_t *list = dn_list_append (NULL, strdup ("a"));
	list = dn_list_append (list, strdup ("b"));
	list = dn_list_append (list, strdup ("c"));

	dn_list_ex_for_each_free (&list, free_callback);
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

RESULT
test_list_ex_foreach_free (void)
{
	uint32_t value1 = 1;
	uint32_t value2 = 2;
	uint32_t value3 = 3;

	dn_list_t *list = dn_list_append (NULL, &value1);
	list = dn_list_append (list, &value2);
	list = dn_list_append (list, &value3);

	dn_list_ex_for_each_free (&list, foreach_clear_free_callback);
	if (list != NULL)
		return FAILED ("ex_foreach_free failed #1");

	if (value1 != 0 || value2 != 0 || value3 != 0)
		return FAILED ("ex_foreach_free failed #2");

	return OK;
}

static
RESULT
test_list_ex_clear(void)
{
	uint32_t value1 = 1;
	uint32_t value2 = 2;
	uint32_t value3 = 3;

	dn_list_t *list = dn_list_append (NULL, &value1);
	list = dn_list_append (list, &value2);
	list = dn_list_append (list, &value3);

	dn_list_ex_clear (&list);

	if (!dn_list_ex_empty (list))
		return FAILED ("ex_clear failed");

	return OK;
}

static
RESULT
test_list_ex_foreach_clear(void)
{
	uint32_t value1 = 1;
	uint32_t value2 = 2;
	uint32_t value3 = 3;

	dn_list_t *list = dn_list_append (NULL, &value1);
	list = dn_list_append (list, &value2);
	list = dn_list_append (list, &value3);

	dn_list_ex_for_each_clear (&list, foreach_clear_free_callback);
	if (list != NULL)
		return FAILED ("ex_foreach_clear failed #1");

	if (value1 != 0 || value2 != 0 || value3 != 0)
		return FAILED ("ex_foreach_clear failed #2");

	return OK;
}

static
RESULT
test_list_ex_foreach_it (void)
{
	uint32_t count = 0;
	dn_list_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_list_append (list, INT32_TO_POINTER (i));

	DN_LIST_EX_FOREACH_BEGIN (list, void *, value) {
		if (POINTER_TO_INT32 (value) != count)
			return FAILED ("foreach iterator failed #1");
		count++;
	} DN_LIST_EX_FOREACH_END;

	if (count != dn_list_length (list))
		return FAILED ("foreach iterator failed #2");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_ex_foreach_rev_it (void)
{
	uint32_t count = N_ELEMS;
	dn_list_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_list_append (list, INT32_TO_POINTER (i));

	DN_LIST_EX_FOREACH_RBEGIN (list, void *, value) {
		if (POINTER_TO_INT32 (value) != count - 1)
			return FAILED ("foreach reverse iterator failed #1");
		count--;
	} DN_LIST_EX_FOREACH_END;

	if (count != 0)
		return FAILED ("foreach reverse iterator failed #2");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_ex_push_back (void)
{
	dn_list_t *list = NULL;
	dn_list_ex_push_back (&list, strdup ("a"));

	dn_list_t *list_2 = list;
	dn_list_ex_push_back (&list_2, strdup ("b"));
	if (list != list_2)
		return FAILED ("ex_push_back failed #1");

	dn_list_t *list_3 = list_2;
	dn_list_ex_push_back (&list_3, strdup ("c"));
	if (list_2 != list_3)
		return FAILED ("ex_push_back failed #2");

	char * value = dn_list_ex_data (dn_list_last (list), char *);
	if (!value || strcmp (value, "c"))
		return FAILED ("ex_push_back failed #3");

	dn_list_ex_for_each_free (&list_3, free_callback);
	if (list_3 != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_list_ex_push_front (void)
{
	dn_list_t *list = NULL;
	dn_list_ex_push_front (&list, strdup ("a"));

	dn_list_t *list_2 = list;
	dn_list_ex_push_front (&list_2, strdup ("b"));
	if (list == list_2)
		return FAILED ("ex_push_back failed #1");

	dn_list_t *list_3 = list_2;
	dn_list_ex_push_front (&list_3, strdup ("c"));
	if (list_2 == list_3)
		return FAILED ("ex_push_back failed #2");

	char * value = dn_list_ex_data (dn_list_first (list), char *);
	if (!value || strcmp (value, "c"))
		return FAILED ("ex_push_front failed #3");

	dn_list_ex_for_each_free (&list_3, free_callback);
	if (list_3 != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_list_ex_erase (void)
{
	const char *a = "a";

	dn_list_t *list = dn_list_append (NULL, (void *)a);
	list = dn_list_append (list, strdup ("b"));
	list = dn_list_append (list, strdup ("c"));

	dn_list_t *list_2 = list;
	dn_list_ex_erase (&list_2, a);
	if (list == list_2)
		return FAILED ("ex_remove failed");

	dn_list_ex_for_each_free (&list_2, free_callback);
	if (list_2 != NULL)
		return FAILED ("ex_free failed");

	return OK;
}

static
RESULT
test_list_ex_find (void)
{
	dn_list_t *list = dn_list_prepend (NULL, (char*)"three");
	bool result = false;
	dn_list_t *found;
	char *data;

	list = dn_list_prepend (list, (char*)"two");
	list = dn_list_prepend (list, (char*)"one");

	data = (char*)"four";
	list = dn_list_append (list, data);

	result = dn_list_ex_find (list, data, &found);

	if (!result || found->data != data)
		return FAILED ("find failed");

	dn_list_free (list);
	return OK;
}

static
RESULT
test_list_ex_find_custom (void)
{
	dn_list_t *list = NULL, *found;
	bool result = false;
	char *foo = (char*)"foo";
	char *bar = (char*)"bar";
	char *baz = (char*)"baz";

	list = dn_list_prepend (list, baz);
	list = dn_list_prepend (list, bar);
	list = dn_list_prepend (list, foo);

	result = dn_list_ex_find_custom (list, baz, find_custom, &found);

	if (!result ||found == NULL)
		return FAILED ("Find failed");

	dn_list_free (list);

	return OK;
}

static
RESULT
test_list_ex_empty (void)
{
	dn_list_t *list = NULL;

	for (uint32_t i = 0; i < N_ELEMS; ++i)
		list = dn_list_append (list, INT32_TO_POINTER (i));

	if (dn_list_ex_empty (list))
		return FAILED ("is_empty failed #1");

	dn_list_ex_free (&list);

	if (!dn_list_ex_empty (list))
		return FAILED ("is_empty failed #2");

	return OK;
}

static
RESULT
test_list_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&dn_list_memory_end_snapshot);
	if ( _CrtMemDifference(&dn_list_memory_diff_snapshot, &dn_list_memory_start_snapshot, &dn_list_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &dn_list_memory_diff_snapshot );
		return FAILED ("memory leak detected!");
	}
#endif
	return OK;
}

static Test dn_list_tests [] = {
	{"test_list_setup", test_list_setup},
	{"test_list_length", test_list_length},
	{"test_list_nth", test_list_nth},
	{"test_list_nth_data", test_list_nth_data},
	{"test_list_index", test_list_index},
	{"test_list_last", test_list_last},
	{"test_list_append", test_list_append},
	{"test_list_concat", test_list_concat},
	{"test_list_find", test_list_find},
	{"test_list_find_custom", test_list_find_custom},
	{"test_list_remove", test_list_remove},
	{"test_list_remove_link", test_list_remove_link},
	{"test_list_insert_sorted", test_list_insert_sorted},
	{"test_list_insert_before", test_list_insert_before},
	{"test_list_sort", test_list_sort},
	{"test_list_copy", test_list_copy},
	{"test_list_foreach", test_list_foreach},
	{"test_list_remove_all", test_list_remove_all},
	{"test_list_reverse", test_list_reverse},
	{"test_list_delete_link", test_list_delete_link},
	{"test_list_next", test_list_next},
	{"test_list_ex_free",test_list_ex_free},
	{"test_list_ex_foreach_free",test_list_ex_foreach_free},
	{"test_list_ex_foreach_clear",test_list_ex_foreach_clear},
	{"test_list_ex_foreach_it",test_list_ex_foreach_it},
	{"test_list_ex_foreach_rev_it",test_list_ex_foreach_rev_it},
	{"test_list_ex_push_back",test_list_ex_push_back},
	{"test_list_ex_push_front",test_list_ex_push_front},
	{"test_list_ex_erase",test_list_ex_erase},
	{"test_list_ex_find",test_list_ex_find},
	{"test_list_ex_find_custom",test_list_ex_find_custom},
	{"test_list_ex_empty",test_list_ex_empty},
	{"test_list_teardown", test_list_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(dn_list_tests_init, dn_list_tests)
