#if defined(_MSC_VER) && defined(_DEBUG)
#include "ep-tests-debug.h"
#endif

#include <eventpipe/ep.h>
#include <eventpipe/ep-session.h>
#include <eventpipe/ep-thread.h>
#include <eglib/test/test.h>

#define TEST_FILE "./ep_test_create_file.txt"

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState eventpipe_memory_start_snapshot;
static _CrtMemState eventpipe_memory_end_snapshot;
static _CrtMemState eventpipe_memory_diff_snapshot;
#endif

extern void ep_rt_mono_thread_exited (void);

static RESULT
test_thread_setup (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&eventpipe_memory_start_snapshot);
#endif
	return NULL;
}

static RESULT
test_create_free_thread (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;

	EventPipeThread *thread = ep_thread_alloc ();
	if (!thread) {
		result = FAILED ("Failed to create thread");
		ep_raise_error ();
	}

ep_on_exit:
	ep_thread_free (thread);
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_addref_release_thread (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;

	EventPipeThread *thread = ep_thread_alloc ();
	if (!thread) {
		result = FAILED ("Failed to create thread");
		ep_raise_error ();
	}

	test_location = 1;

	if (ep_rt_volatile_load_uint32_t ((const volatile uint32_t *)ep_thread_get_ref_count_ref (thread)) != 0) {
		result = FAILED ("Ref count should start at 0");
		ep_raise_error ();
	}

	test_location = 2;

	ep_thread_addref (thread);

	if (ep_rt_volatile_load_uint32_t ((const volatile uint32_t *)ep_thread_get_ref_count_ref (thread)) != 1) {
		result = FAILED ("addref should increment 1");
		ep_raise_error ();
	}

	test_location = 3;

	ep_thread_release (thread);
	thread = NULL;

ep_on_exit:
	ep_thread_free (thread);
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_get_or_create_thread (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;

	EventPipeThread *thread = ep_thread_get ();
	if (thread) {
		result = FAILED ("ep_thread_get should return NULL");
		ep_raise_error ();
	}

	test_location = 1;

	thread = ep_thread_get_or_create ();
	if (!thread) {
		result = FAILED ("ep_thread_get_or_create should not return NULL");
		ep_raise_error ();
	}

	test_location = 2;

	thread = ep_thread_get ();
	if (!thread) {
		result = FAILED ("ep_thread_get should not return NULL");
		ep_raise_error ();
	}

	test_location = 3;

	if (ep_rt_volatile_load_uint32_t ((const volatile uint32_t *)ep_thread_get_ref_count_ref (thread)) == 0) {
		result = FAILED ("thread ref count should not be 0");
		ep_raise_error ();
	}

	test_location = 4;

	// Need to emulate a thread exit to make sure TLS gets cleaned up for current thread
	// or we will get memory leaks reported.
	ep_rt_mono_thread_exited ();

	thread = ep_thread_get ();
	if (thread) {
		result = FAILED ("ep_thread_get should return NULL");
		ep_raise_error ();
	}

ep_on_exit:
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_thread_activity_id (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;

	uint8_t empty_id [EP_ACTIVITY_ID_SIZE] = {0};
	uint8_t current_activity_id [EP_ACTIVITY_ID_SIZE];
	uint8_t new_activity_id [EP_ACTIVITY_ID_SIZE];

	ep_thread_create_activity_id (new_activity_id, sizeof (new_activity_id));
	if (!memcmp (empty_id, new_activity_id, sizeof (new_activity_id))) {
		result = FAILED ("Created activity id is empty");
		ep_raise_error ();
	}

	test_location = 1;

	EventPipeThread *thread = ep_thread_get ();
	if (thread) {
		result = FAILED ("ep_thread_get should return NULL");
		ep_raise_error ();
	}

	test_location = 2;

	thread = ep_thread_get_or_create ();
	if (!thread) {
		result = FAILED ("ep_thread_get_or_create should not return NULL");
		ep_raise_error ();
	}

	test_location = 3;

	ep_thread_get_activity_id (thread, current_activity_id, sizeof (current_activity_id));
	if (memcmp (empty_id, current_activity_id, sizeof (current_activity_id))) {
		result = FAILED ("Current activity id is not empty");
		ep_raise_error ();
	}

	test_location = 4;

	ep_thread_set_activity_id (thread, new_activity_id, sizeof (new_activity_id));

	ep_thread_get_activity_id (thread, current_activity_id, sizeof (current_activity_id));
	if (memcmp (new_activity_id, current_activity_id, sizeof (current_activity_id))) {
		result = FAILED ("Current activity id doesn't match previously set activity id");
		ep_raise_error ();
	}

	test_location = 5;

	// Need to emulate a thread exit to make sure TLS gets cleaned up for current thread
	// or we will get memory leaks reported.
	extern void ep_rt_mono_thread_exited (void);
	ep_rt_mono_thread_exited ();

	thread = ep_thread_get ();
	if (thread) {
		result = FAILED ("ep_thread_get should return NULL");
		ep_raise_error ();
	}

ep_on_exit:
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_thread_is_rundown_thread (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;

	EventPipeThread *thread = ep_thread_alloc ();
	if (!thread) {
		result = FAILED ("Failed to create thread");
		ep_raise_error ();
	}

	test_location = 1;

	if (ep_thread_is_rundown_thread (thread)) {
		result = FAILED ("Thread is a rundown thread");
		ep_raise_error ();
	}

	test_location = 2;

	EventPipeSession dummy_session;
	ep_thread_set_as_rundown_thread (thread, &dummy_session);
	if (!ep_thread_is_rundown_thread (thread)) {
		result = FAILED ("Thread is not a rundown thread");
		ep_raise_error ();
	}

	test_location = 3;

	if (ep_thread_get_rundown_session (thread) != &dummy_session) {
		result = FAILED ("Unexpected rundown session");
		ep_raise_error ();
	}

	test_location = 4;

	ep_thread_set_as_rundown_thread (thread, NULL);

	if (ep_thread_is_rundown_thread (thread)) {
		result = FAILED ("Thread is a rundown thread");
		ep_raise_error ();
	}

ep_on_exit:
	ep_thread_free (thread);
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_thread_session_write (void)
{
	RESULT result = NULL;
	uint32_t test_location = 0;

	EventPipeThread *thread = ep_thread_alloc ();
	if (!thread) {
		result = FAILED ("Failed to create thread");
		ep_raise_error ();
	}

	test_location = 1;

	uint32_t session_write = ep_thread_get_session_use_in_progress (thread);
	if (session_write < EP_MAX_NUMBER_OF_SESSIONS) {
		result = FAILED ("Session is in use");
		ep_raise_error ();
	}

	test_location = 2;

	ep_thread_set_session_use_in_progress (thread, 1);

	session_write = ep_thread_get_session_use_in_progress (thread);
	if (session_write != 1) {
		result = FAILED ("Wrong session id in use");
		ep_raise_error ();
	}

	test_location = 3;

	ep_thread_set_session_use_in_progress (thread, UINT32_MAX);

	session_write = ep_thread_get_session_use_in_progress (thread);
	if (session_write != UINT32_MAX) {
		result = FAILED ("Session is in use");
		ep_raise_error ();
	}

ep_on_exit:
	ep_thread_free (thread);
	return result;

ep_on_error:
	if (!result)
		result = FAILED ("Failed at test location=%i", test_location);
	ep_exit_error_handler ();
}

static RESULT
test_thread_teardown (void)
{
#ifdef _CRTDBG_MAP_ALLOC
	_CrtMemCheckpoint (&eventpipe_memory_end_snapshot);
	if ( _CrtMemDifference( &eventpipe_memory_diff_snapshot, &eventpipe_memory_start_snapshot, &eventpipe_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &eventpipe_memory_diff_snapshot );
		return FAILED ("Memory leak detected!");
	}
#endif
	return NULL;
}

static Test ep_thread_tests [] = {
	{"test_thread_setup", test_thread_setup},
	{"test_create_free_thread", test_create_free_thread},
	{"test_addref_release_thread", test_addref_release_thread},
	{"test_get_or_create_thread", test_get_or_create_thread},
	{"test_thread_activity_id", test_thread_activity_id},
	{"test_thread_is_rundown_thread", test_thread_is_rundown_thread},
	{"test_thread_session_write", test_thread_session_write},
	{"test_thread_teardown", test_thread_teardown},
	{NULL, NULL}
};

DEFINE_TEST_GROUP_INIT(ep_thread_tests_init, ep_thread_tests)
